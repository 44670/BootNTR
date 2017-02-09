#include "main.h"
#include "gen.h"

#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wunused-variable"



#define MAX_MAP_SIZE (0x00400000)

NTR_CONFIG g_ntrConfig = {0};
BOOTNTR_CONFIG g_bnConfig = {0};

NTR_CONFIG* ntrConfig;
BOOTNTR_CONFIG* bnConfig;
u8* tmpBuffer;
#define TMPBUFFER_SIZE  (0x20000)

void backdoorHandler();





void doWait() {
	svcSleepThread(500000000);
}


void showMsg(char* str) {
	printf("%s\n", str);
}

void showMsgPaused(char* str) {
	showMsg(str);
	doWait();
}

u32 protectRemoteMemory(Handle hProcess, void* addr, u32 size) {
	u32 outAddr = 0;
	
	return svc_controlProcessMemory(hProcess, addr, addr, size, 6, 7);
}

u32 copyRemoteMemory(Handle hDst, void* ptrDst, Handle hSrc, void* ptrSrc, u32 size) {
	u32 dmaConfig[20] = {0};
	u32 hdma = 0;
	u32 state, i, ret;
	
	ret = svc_flushProcessDataCache(hSrc, (u32)ptrSrc, size);
	if (ret != 0) {
		return ret;
	}
	ret = svc_flushProcessDataCache(hDst, (u32)ptrDst, size);
	if (ret != 0) {
		return ret;
	}
	
	ret = svc_startInterProcessDma(&hdma, hDst, ptrDst, hSrc, ptrSrc, size, dmaConfig);
	if (ret != 0) {
		return ret;
	}
	state = 0;
	

	
	if (g_ntrConfig.InterProcessDmaFinishState == 0) {
		ret = svc_getDmaState(&state, hdma);
		svc_sleepThread(1000000000);
		ret = svc_getDmaState(&state, hdma);
		g_ntrConfig.InterProcessDmaFinishState = state;
		printf("InterProcessDmaFinishState: %08x\n", state);
	} else {
		for (i = 0; i < 10000; i++ ) {
			state = 0;
			ret = svc_getDmaState(&state, hdma);
			if (state == g_ntrConfig.InterProcessDmaFinishState) {
				break;
			}
			svc_sleepThread(1000000);
		}

		if (i >= 10000) {
			printf("readRemoteMemory time out %08x\n", state);
			return RESULT_ERROR;
		}
	}
	
	svc_closeHandle(hdma);
	ret = svc_invalidateProcessDataCache(hDst, (u32)ptrDst, size);
	if (ret != 0) {
		return ret;
	}
	return 0;
}


u32 patchRemoteProcess(u32 pid, u32 addr, u8* buf, u32 len) {
	u32 hProcess, hFile, ret, i, state, t;
	ret = svc_openProcess(&hProcess, pid);
	if (ret != 0) {
		showMsgPaused("openProcess failed");
		hProcess = 0;
		goto final;
	}
	ret = protectRemoteMemory(hProcess, (void*)((addr / 0x1000) * 0x1000), 0x1000);
	if (ret != 0) {
		showMsgPaused("protect failed");
		goto final;
	}
	ret = copyRemoteMemory(hProcess, (void*)addr, 0xffff8001, buf, len);
	if (ret != 0) {
		showMsgPaused("copy failed");
		goto final;
	}
	final:
	if (hProcess) {
		svc_closeHandle(hProcess);
	}
	return ret;
}


u32 getCurrentProcessHandle() {
	u32 handle = 0;
	u32 ret;
	u32 hCurrentProcess;
	u32 currentPid;
	
	svc_getProcessId(&currentPid, 0xffff8001);
	ret = svc_openProcess(&handle, currentPid);
	if (ret != 0) {
		return 0;
	}
	hCurrentProcess = handle;
	return hCurrentProcess;
}


u32 rtAlignToPageSize(u32 size) {
	return ((size / 0x1000) + 1) * 0x1000;
}

u32 rtGetPageOfAddress(u32 addr) {
	return (addr / 0x1000) * 0x1000;
}


u32 rtCheckRemoteMemoryRegionSafeForWrite(Handle hProcess, u32 addr, u32 size) {
	u32 ret;
	u32 startPage, endPage, page;
	
	startPage = rtGetPageOfAddress(addr);
	endPage = rtGetPageOfAddress(addr + size - 1);

	for (page = startPage; page <= endPage; page += 0x1000) {
		ret = protectRemoteMemory(hProcess, (void*) page, 0x1000);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}



void bnInitParamsByFirmware();


u32 findNearestSTMFD(u32 base, u32 pos) {
	if (pos < base) {
		return 0;
	}
	pos = pos - pos % 4;
	u32 term = pos - 0x1000;
	if (term < base) {
		term = base;
	}
	while (pos >= term) {
		if (*(u16*)(pos + 2) == 0xe92d){
			return pos;
		}
		pos -= 4;
	}
	return 0;
}

u32 searchBytes(u32 startAddr, u32 endAddr, u8* pat, int patlen, int step) {
	u32 lastPage = 0;
	u32 pat0 = ((u32*)pat)[0];

	while (1) {
		if (startAddr + patlen >= endAddr) {
				return 0;
		}
		if (*((u32*)(startAddr)) == pat0) {
			if (memcmp((u32*) startAddr, pat, patlen) == 0) {
				return startAddr;
			}
		}
		startAddr += step;
	}
	return 0;
}

u32 locateSwapBuffer(u32 startAddr, u32 endAddr) {
	
	static u32 pat[] = { 0xe1833000, 0xe2044cff, 0xe3c33cff, 0xe1833004, 0xe1824f93 };
	static u32 pat2[] = { 0xe8830e60, 0xee078f9a, 0xe3a03001, 0xe7902104 };
	static u32 pat3[] = { 0xee076f9a, 0xe3a02001, 0xe7901104, 0xe1911f9f, 0xe3c110ff};

	u32 addr = searchBytes(startAddr, endAddr, pat, sizeof(pat), 4);
	if (!addr) {
		addr = searchBytes(startAddr, endAddr, pat2, sizeof(pat2), 4);
	}
	if (!addr) {
		addr = searchBytes(startAddr, endAddr, pat3, sizeof(pat3), 4);
	}
	return findNearestSTMFD(startAddr, addr);
}

u32 translateAddr(u32 mappedAddr) {
	if (mappedAddr < 0x0f000000) {
		return 0;
	}
	return mappedAddr - 0x0f000000 + 0x00100000;
}

Result analyseHomeMenu()
{
	Result ret=0;
	u32 text = 0x0f000000;
	u32 ampxi_funcoffset = 0;
	u32 mapSize = MAX_MAP_SIZE;

	MemInfo meminfo;
	PageInfo pageinfo;

	memset(&meminfo, 0, sizeof(meminfo));
	memset(&pageinfo, 0, sizeof(pageinfo));

	ret = svcQueryMemory(&meminfo, &pageinfo, (u32)text);
	if(R_FAILED(ret))
	{
		printf("svcQueryMemory failed: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	if(meminfo.size < mapSize) mapSize = meminfo.size;

	printf("mapSize: %08x, size: %08x\n", mapSize, meminfo.size);

	static u8 patFsRead[] = {0xC2, 0x00, 0x02, 0x08};
	ntrConfig->HomeFSReadAddr = translateAddr(findNearestSTMFD(text ,searchBytes(text, text + mapSize, patFsRead, sizeof(patFsRead), 4)));
	printf("HomeFSReadAddr: %08x\n", ntrConfig->HomeFSReadAddr);

	static u8 patFsHandle[] = {0xf9, 0x67, 0xa0, 0x08};
	u32 t = searchBytes(text, text + mapSize, patFsHandle, sizeof(patFsHandle), 4);
	if (t > 0) {
		ntrConfig->HomeFSUHandleAddr = *(u32*)(t-4);
	}
	printf("HomeFSUHandleAddr: %08x\n", ntrConfig->HomeFSUHandleAddr);

	static u8 patCartUpdate[] = {0x42, 0x00, 0x07, 0x00};
	ntrConfig->HomeCardUpdateInitAddr = translateAddr(findNearestSTMFD(text ,searchBytes(text, text + mapSize, patCartUpdate, sizeof(patCartUpdate), 4)));
	printf("HomeCardUpdateInitAddr: %08x\n", ntrConfig->HomeCardUpdateInitAddr);

	static u8 patStartApplet[] = {0x40, 0x01, 0x15, 0x00};
	ntrConfig->HomeAptStartAppletAddr = translateAddr(findNearestSTMFD(text ,searchBytes(text, text + mapSize, patStartApplet, sizeof(patStartApplet), 4)));
	printf("HomeAptStartAppletAddr: %08x\n", ntrConfig->HomeAptStartAppletAddr);

	ntrConfig->HomeMenuInjectAddr = translateAddr(locateSwapBuffer(text, text + mapSize));
	printf("HomeMenuInjectAddr: %08x\n", ntrConfig->HomeMenuInjectAddr);


	return 0;
}

Result bnInitParamsByHomeMenu() {
	u32 hProcess = 0, ret;
	vu32 t = 0x11111111;
	u8 region;

	ret = svc_openProcess(&hProcess, ntrConfig->HomeMenuPid);
	if (ret != 0) {
		printf("openProcess failed:%08x\n", ret);
		return ret;
	}
	//Map .text+0(0x00100000) up to 0x40000, to vaddr 0x0f000000 in the current process.
	ret = svcMapProcessMemory(hProcess, 0x0f000000, MAX_MAP_SIZE);
	if (ret != 0) {
		printf("map process memory failed:%08x\n", ret);
		goto final;
	}
	analyseHomeMenu();
	svcUnmapProcessMemory(hProcess, 0x0f000000, MAX_MAP_SIZE);
final:
	svc_closeHandle(hProcess);
	return ret;
}


void flushDataCache() {
	u32* ptr = (u32*) tmpBuffer;
	u32 i;
	for (i = 0; i < (0x20000 / 4); i++) {
		ptr[i] += 0x11111111;
	}
}



Result bnPatchAccessCheck() {
	Result ret;

	showMsgPaused("patching svc check");
	svcBackdoor(backdoorHandler);
	showMsgPaused("svc check patched");

	
	// do a dma copy to get the finish state value on current console
	ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, tmpBuffer, CURRENT_PROCESS_HANDLE, tmpBuffer + 0x10, 0x10);
	if (ret != 0) {
		printf("copyRemoteMemory failed: %08x\n", ret);
		return RESULT_ERROR;
	}

	
	u8 smPatchBuf[] = {0x02,0x00,0xA0,0xE3,0x1E,0xFF,0x2F,0xE1};
	ret = patchRemoteProcess(bnConfig->SMPid, bnConfig->SMPatchAddr, smPatchBuf, 8);
	if (ret != 0) {
		showMsgPaused("SM patch failed");
		return ret;
	}
	showMsgPaused("SM patched");
	u32 fsPatchValue = 0x47702001; 
	ret = patchRemoteProcess(bnConfig->FSPid, bnConfig->FSPatchAddr, (u8*) &fsPatchValue, 4);
	if (ret != 0) {
		showMsgPaused("FS patch failed");
		return ret;
	}
	showMsgPaused("FS patched");
	return 0;
}





Result bnLoadAndExecuteNTR() {
	u32 ret;
	
	
//	fsInit();
	FILE *file = fopen("sdmc:/ntr.bin","rb");
	if (file == 0) {
		printf("open ntr.bin failed\n");
		return RESULT_ERROR;
	}
	fseek(file,0,SEEK_END);
	u32 size = ftell(file);
	if (size == 0) {
		printf("size == 0\n");
		return RESULT_ERROR;
	}
	printf("size: %08x\n", size);
	fseek(file,0,SEEK_SET);
	ntrConfig->arm11BinSize = rtAlignToPageSize(size);
	u32 outAddr;
	u32 totalSize = (ntrConfig->arm11BinSize) * 2;
	// use linearMemAlign instead of svc_controlMemory
	outAddr = (u32)linearMemAlign(totalSize, 0x1000);
	if (outAddr == 0) {
		printf("linearMemAlign failed\n");
		return RESULT_ERROR;
	}
	
	ntrConfig->arm11BinStart = (outAddr + ntrConfig->arm11BinSize);
	rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), outAddr, totalSize);
	printf("outAddr: %08x\n", outAddr);
	memset((void*) outAddr, 0, totalSize);
	fread((void*) outAddr, size, 1, file);
	memcpy((void*) (outAddr +(ntrConfig->arm11BinSize)), (void*) outAddr, size);
//	fsExit();
	
	Handle fsUserHandle = 0;
	ret=srvGetServiceHandle(&fsUserHandle, "fs:USER");
	FSUSER_Initialize(fsUserHandle);
	ntrConfig->fsUserHandle = fsUserHandle;
	
	u32* bootArgs = outAddr + 4;
	bootArgs[0] = 0;
	bootArgs[1] = 0xb00d;
	bootArgs[2] = ntrConfig;
	typedef void (*funcType)();
	((funcType)(outAddr))();
	
	svcCloseHandle(fsUserHandle);
	
	return 0;
}

#define VALIDATE_PARAM(a) if ((a) == 0) { printf("param is missing: %s\n", #a); return RESULT_ERROR; }

Result validateFirmParams() {
	VALIDATE_PARAM(ntrConfig->PMSvcRunAddr);
	VALIDATE_PARAM(bnConfig->SvcPatchAddr);
	VALIDATE_PARAM(bnConfig->FSPatchAddr);
	VALIDATE_PARAM(bnConfig->SMPatchAddr);
	VALIDATE_PARAM(ntrConfig->IoBasePad); 
	VALIDATE_PARAM(ntrConfig->IoBaseLcd);
	VALIDATE_PARAM(ntrConfig->IoBasePdc);
	VALIDATE_PARAM(ntrConfig->KMMUHaxAddr);
	VALIDATE_PARAM(ntrConfig->KMMUHaxSize);
	VALIDATE_PARAM(ntrConfig->KProcessHandleDataOffset);
	VALIDATE_PARAM(ntrConfig->KProcessPIDOffset);
	VALIDATE_PARAM(ntrConfig->KProcessCodesetOffset);
	return 0;
}

Result validateHomeMenuParams() {
	VALIDATE_PARAM(ntrConfig->HomeMenuInjectAddr);
	VALIDATE_PARAM(ntrConfig->HomeFSReadAddr);
	VALIDATE_PARAM(ntrConfig->HomeCardUpdateInitAddr);
	VALIDATE_PARAM(ntrConfig->HomeFSUHandleAddr);
	VALIDATE_PARAM(ntrConfig->HomeAptStartAppletAddr);
	return 0;
}

int waitKey() {
	showMsg("Waiting for user input...");
		// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown) {
			return kDown;
		}
		
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	return 0;

}

Result bnBootNTR() {	
	Result ret;
	

	// allocate the tmpBuffer
	// use linearMemAlign instead of svc_controlMemory
	tmpBuffer = (u32)linearMemAlign(TMPBUFFER_SIZE, 0x1000);
	if (tmpBuffer == 0) {
		printf("linearMemAlign failed\n");
		return RESULT_ERROR;
	}

	printf("tmpBuffer: %08x\n", tmpBuffer);
	
	bnInitParamsByFirmware();
	printf("IsNew3DS: %d\n", ntrConfig->isNew3DS);
	printf("firmVersion: %08x\n", ntrConfig->firmVersion);
	if (validateFirmParams() != 0) {
		// FIRM Params is not complete, firmware should be considered as unsupported
		ntrConfig->firmVersion = 0;
	}
	ntrConfig->ShowDbgFunc = (void*) showMsg;
	

	
	/*
	
	if (bnConfig->requireKernelHax) {
		ret = khaxInit();
		if (ret != 0) {
			printf("khaxInit failed: %08x\n", ret);
			return ret;
		}
		showMsgPaused("khaxInit OK");
	}
	*/


	
	if (ntrConfig->firmVersion) {

		ret = bnPatchAccessCheck(bnConfig);
	
		if (ret != 0) {
			printf("bnPatchAccessCheck failed: %08x\n", ret);
			return ret;
		}
		showMsgPaused("bnPatchAccessCheck OK");
	
	} else {
		showMsgPaused("ERROR: unknown FIRM, access-checks could not be patched");
	}

	ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, tmpBuffer, CURRENT_PROCESS_HANDLE, tmpBuffer + 0x10, 0x10);
	if (ret != 0) {
		printf("copyRemoteMemory failed: %08x\n", ret);
		return RESULT_ERROR;
	}
	rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), tmpBuffer, TMPBUFFER_SIZE);
	showMsgPaused("copyRemoteMemory/controlProcessMemory OK");
	


	bnInitParamsByHomeMenu();
	ntrConfig->HomeMenuVersion = SYSTEM_VERSION(1, 0, 0);
	if (validateHomeMenuParams() != 0) {
		// Home menu Params is not complete, should be considered as unsupported
		ntrConfig->HomeMenuVersion = 0;
	}
	if (ntrConfig->HomeMenuVersion == 0) {
		showMsgPaused("ERROR: unknown Home Menu");
	} 

	// load and execute ntr.bin, if firmware/home menu is unsupported, it will start ram dumping
	if ((ntrConfig->HomeMenuVersion == 0)  || (ntrConfig->firmVersion == 0)) {
		showMsg("Press X if you want to start ram dumping.");
		int key = waitKey();
		if (key & KEY_X) {
			ret = bnLoadAndExecuteNTR();
		}
		ret = RESULT_ERROR;
	} else {
		ret = bnLoadAndExecuteNTR();
	}
	
	return ret;
}

int main() {
	Result ret;
	int isSuccess = 0;
	
	// Initialize services
	gfxInitDefault();

	// Init console for text output
	consoleInit(GFX_BOTTOM, NULL);


	printf("BootNTR 3.0\n");
	ntrConfig = &g_ntrConfig;
	bnConfig = &g_bnConfig;
	ret = bnBootNTR();
	if (ret == 0) {
		printf("NTR CFW loaded successfully\n");
		svcSleepThread(1000000000);
		isSuccess = 1;
	} else {
		printf("Boot NTR CFW failed\n");
	}
	int autoExit = 0;
	if (isSuccess && (APP_MEMTYPE == 0)) {
		autoExit = 1;
	}
	if (autoExit) {
		printf("Exiting...\n");
		goto final;
	}
	printf("Press Home button to return to the menu.\n");

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) {
			break; // break in order to return to hbmenu
		}
		
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

final:

	gfxExit();
	return 0;
}
