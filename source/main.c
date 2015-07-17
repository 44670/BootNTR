#include <stdio.h>
#include <3ds.h>
#include <khax.h>
#include "ntr_config.h"
#include "mysvcs.h"
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define CURRENT_PROCESS_HANDLE	(0xffff8001)
#define RESULT_ERROR (1)

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



void bnInitParamsByFirmware() {
	u32 kernelVersion = osGetKernelVersion();
	u32 isNew3DS = 0;
	if (kernelVersion >= SYSTEM_VERSION(2, 44, 6))
	{
		u8 tmp;
		
		Result ret = APT_CheckNew3DS(0, &tmp);;
		if (ret == 0)
		{
			if (tmp) {
				isNew3DS = 1;
			}
		} 
	}
	ntrConfig->isNew3DS = isNew3DS;
	ntrConfig->PMPid = 2;
	ntrConfig->HomeMenuPid = 0xf;
	bnConfig->SMPid = 3;
	bnConfig->FSPid = 0;
	
	if (!isNew3DS) {
		if (kernelVersion == SYSTEM_VERSION(2, 44, 6)) {
			//TODO: add old3ds 8.0.0 firmware support 
			ntrConfig->firmVersion = SYSTEM_VERSION(8, 0, 0);
			bnConfig->SvcPatchAddr = 0xDFF82294;
		}
		if (kernelVersion == SYSTEM_VERSION(2, 46, 0)) {
			// old3ds 9.0.0
			ntrConfig->firmVersion = SYSTEM_VERSION(9, 0, 0);
			ntrConfig->PMSvcRunAddr = 0x00102FC0;
			ntrConfig->ControlMemoryPatchAddr1 = 0xdff882cc;
			ntrConfig->ControlMemoryPatchAddr2 = 0xdff882d0;
			
			bnConfig->SvcPatchAddr = 0xDFF82290;
			bnConfig->FSPatchAddr = 0x0010ED64;
			bnConfig->SMPatchAddr = 0x00101838;
			
			ntrConfig->IoBasePad = 0xfffc6000;
			ntrConfig->IoBaseLcd = 0xfffc8000;
			ntrConfig->IoBasePdc = 0xfffc0000;
			ntrConfig->KMMUHaxAddr = 0xfffbe000;
			ntrConfig->KMMUHaxSize = 0x00010000;
			ntrConfig->KProcessHandleDataOffset = 0xD4;
			ntrConfig->KProcessPIDOffset = 0xB4;
			ntrConfig->KProcessCodesetOffset = 0xB0;
			
		}
	} else {
		ntrConfig->IoBasePad = 0xfffc2000;
		ntrConfig->IoBaseLcd = 0xfffc4000;
		ntrConfig->IoBasePdc = 0xfffbc000;
		ntrConfig->KMMUHaxAddr = 0xfffba000;
		ntrConfig->KMMUHaxSize = 0x00010000;
		ntrConfig->KProcessHandleDataOffset = 0xdc;
		ntrConfig->KProcessPIDOffset = 0xBC;
		ntrConfig->KProcessCodesetOffset = 0xB8;
			
		if (kernelVersion == SYSTEM_VERSION(2, 45, 5)) {
			// new3ds 8.1
			ntrConfig->firmVersion = SYSTEM_VERSION(8, 1, 0);
			ntrConfig->PMSvcRunAddr = 0x0010308C;
			ntrConfig->ControlMemoryPatchAddr1 = 0xdff88158;
			ntrConfig->ControlMemoryPatchAddr2 = 0xdff8815C;
			
			bnConfig->SvcPatchAddr = 0xDFF82264;
			bnConfig->FSPatchAddr = 0x0010ED64;
			bnConfig->SMPatchAddr = 0x00101838;
		}
		if (kernelVersion == SYSTEM_VERSION(2, 46, 0)) {
			// new3ds 9.0
			ntrConfig->firmVersion = SYSTEM_VERSION(9, 0, 0);
			ntrConfig->PMSvcRunAddr = 0x00102FEC;
			ntrConfig->ControlMemoryPatchAddr1 = 0xdff884ec;
			ntrConfig->ControlMemoryPatchAddr2 = 0xdff884f0;
			
			bnConfig->SvcPatchAddr = 0xDFF82260;
			bnConfig->FSPatchAddr = 0x0010ED64;
			bnConfig->SMPatchAddr = 0x00101838;
		}
		
		if (kernelVersion == SYSTEM_VERSION(2, 49, 0)) {
			// new3ds 9.5
			ntrConfig->firmVersion = SYSTEM_VERSION(9, 5, 0);
			ntrConfig->PMSvcRunAddr = 0x001030F8 ;
			ntrConfig->ControlMemoryPatchAddr1 = 0xdff884F8;
			ntrConfig->ControlMemoryPatchAddr2 = 0xdff884FC;
			
			bnConfig->SvcPatchAddr = 0xDFF8226c;
			bnConfig->FSPatchAddr = 0x0010ED64;
			bnConfig->SMPatchAddr = 0x00101838 ;
		}
	}
	bnConfig->requireKernelHax = 0;
}

Result bnInitParamsByHomeMenu() {
	u32 hProcess = 0, ret;
	vu32 t = 0x11111111;
	
	ret = svc_openProcess(&hProcess, ntrConfig->HomeMenuPid);
	if (ret != 0) {
		printf("openProcess failed:%08x\n", ret);
		return ret;
	}
	flushDataCache();
	*(u32*)(tmpBuffer) = 0;
	ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, tmpBuffer, hProcess, (void*)0x00200000, 4);
	if (ret != 0) {
		printf("copyRemoteMemory failed:%08x\n", ret);
		return ret;
	}
	svc_closeHandle(hProcess);
	t = *(u32*)(tmpBuffer);
	printf("0x0020000 in HomeMenu: %08x\n", t);
	if (t == 0xe59f80f4) {
		// new3ds 9.2.0
		ntrConfig->HomeMenuVersion = SYSTEM_VERSION(9, 2, 0);;
		ntrConfig->HomeMenuInjectAddr = 0x131208;
		ntrConfig->HomeFSReadAddr = 0x0012F6EC;
		ntrConfig->HomeCardUpdateInitAddr = 0x139900;
		ntrConfig->HomeFSUHandleAddr = 0x002F0EFC;
		ntrConfig->HomeAptStartAppletAddr = 0x00131C98;

	} 
	if (t == 0xE28DD008) {
		// new3ds 9.1.0
		ntrConfig->HomeMenuVersion = SYSTEM_VERSION(9, 1, 0);;
		ntrConfig->HomeMenuInjectAddr = 0x00131208;
		ntrConfig->HomeFSReadAddr = 0x0012F6EC;
		ntrConfig->HomeCardUpdateInitAddr = 0x139900;
		ntrConfig->HomeFSUHandleAddr = 0x002F1EFC;
		ntrConfig->HomeAptStartAppletAddr = 0x00131C98;
	}
	if (t == 0xE1B03F02) {
		// new3ds 9.0.0
		ntrConfig->HomeMenuVersion = SYSTEM_VERSION(9, 0, 0);;
		ntrConfig->HomeMenuInjectAddr = 0x00130CFC;
		ntrConfig->HomeFSReadAddr = 0x0012F224;
		ntrConfig->HomeCardUpdateInitAddr = 0x001393F4;
		ntrConfig->HomeFSUHandleAddr = 0x002EFEFC;
		ntrConfig->HomeAptStartAppletAddr = 0x0013178C;
	}
	if (t == 0xE28F2E19) {
		// new3ds 8.1.0
		ntrConfig->HomeMenuVersion = SYSTEM_VERSION(8, 1, 0);;
		ntrConfig->HomeMenuInjectAddr = 0x00129098;
		ntrConfig->HomeFSReadAddr = 0x0011AAB8;
		ntrConfig->HomeCardUpdateInitAddr = 0x0013339C;
		ntrConfig->HomeFSUHandleAddr = 0x00278E4C;
		ntrConfig->HomeAptStartAppletAddr = 0x00129BFC;
	}
	if (t == 0xe59f201c ) {
		// iQue 9.3.0
		ntrConfig->HomeMenuVersion = SYSTEM_VERSION(9, 3, 0);
		ntrConfig->HomeMenuInjectAddr = 0x13b7b0;
		ntrConfig->HomeFSReadAddr = 0x1188e0;
		ntrConfig->HomeCardUpdateInitAddr = 0x13434c;
		ntrConfig->HomeFSUHandleAddr = 0x2240d4;
		ntrConfig->HomeAptStartAppletAddr = 0x128480;
	}
	if (t == 0xe3a06001 ) {
		// iQue 4.4.0
		ntrConfig->HomeMenuVersion = SYSTEM_VERSION(4, 4, 0);
		ntrConfig->HomeMenuInjectAddr = 0x13c344;
		ntrConfig->HomeFSReadAddr = 0x118888;
		ntrConfig->HomeCardUpdateInitAddr = 0x134448;
		ntrConfig->HomeFSUHandleAddr = 0x2210cc;
		ntrConfig->HomeAptStartAppletAddr = 0x12844c;
	}
	if (t == 0xeb0083b3 ) {
		// new3ds 9.5.0
		ntrConfig->HomeMenuVersion = SYSTEM_VERSION(9, 5, 0);
		ntrConfig->HomeMenuInjectAddr = 0x12e1f8;
		ntrConfig->HomeFSReadAddr = 0x12c624;
		ntrConfig->HomeCardUpdateInitAddr = 0x136a8c;
		ntrConfig->HomeFSUHandleAddr = 0x313f7c;
		ntrConfig->HomeAptStartAppletAddr = 0x12ec88;
	}
	return 0;
}


void flushDataCache() {
	u32* ptr = (u32*) tmpBuffer;
	u32 i;
	for (i = 0; i < (0x20000 / 4); i++) {
		ptr[i] += 0x11111111;
	}
}

u32 kernelParams[32];

void kernelCallback() {
	u32 svc_patch_addr = g_bnConfig.SvcPatchAddr;
	vu32 i;
	
	if (kernelParams[0] == 1) {
		
		InvalidateEntireInstructionCache();
		InvalidateEntireDataCache();

		*(int *)(svc_patch_addr+8) = 0xE1A00000; //NOP
		*(int *)(svc_patch_addr) = 0xE1A00000; //NOP
		InvalidateEntireInstructionCache();
		InvalidateEntireDataCache();
		flushDataCache();
		InvalidateEntireInstructionCache();
		InvalidateEntireDataCache();
	}
}

void testSvcBackdoor() {
	kernelParams[0] = 0;
	svcBackdoor((void*) backdoorHandler);
}

Result bnPatchAccessCheck() {
	Result ret;

	svc_sleepThread(3000000000);
	showMsgPaused("patching svc check");
	kernelParams[0] = 1;
	svcBackdoor((void*) backdoorHandler);
	svc_sleepThread(1000000000);
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
	
	
	fsInit();
	FILE *file = fopen("ntr.bin","rb");
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
	ret = svc_controlMemory((u32*)&outAddr, 0, 0, totalSize, 0x10003, 3);
	if (ret != 0) {
		printf("svc_controlMemory failed: %08x\n", ret);
		return RESULT_ERROR;
	}
	ntrConfig->arm11BinStart = (outAddr + ntrConfig->arm11BinSize);
	rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), outAddr, totalSize);
	printf("outAddr: %08x\n", outAddr);
	memset((void*) outAddr, 0, totalSize);
	fread((void*) outAddr, size, 1, file);
	memcpy((void*) (outAddr +(ntrConfig->arm11BinSize)), (void*) outAddr, size);
	fsExit();
	
	Handle fsUserHandle;
	ret=srvGetServiceHandle(&fsUserHandle, "fs:USER");
	FSUSER_Initialize(&fsUserHandle);
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

Result bnBootNTR() {	
	Result ret;
	

	// allocate the tmpBuffer
	ret = svc_controlMemory((u32*)&tmpBuffer, 0, 0, TMPBUFFER_SIZE, 0x10003, 3);
	if (ret != 0) {
		printf("svc_controlMemory failed: %08x\n", ret);
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
	

	
	if (bnConfig->requireKernelHax) {
		ret = khaxInit();
		if (ret != 0) {
			printf("khaxInit failed: %08x\n", ret);
			return ret;
		}
		showMsgPaused("khaxInit OK");
	}

	


	
	showMsg("testing svcBackdoor");
	testSvcBackdoor();
	showMsgPaused("testSvcBackdoor OK");
	
	
	if (ntrConfig->firmVersion) {

		ret = bnPatchAccessCheck(bnConfig);
	
		if (ret != 0) {
			printf("bnPatchAccessCheck failed: %08x\n", ret);
			return ret;
		}
		showMsgPaused("bnPatchAccessCheck OK");
	
	} else {
		showMsgPaused("unknown FIRM, access-checks could not be patched");
	}

	ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, tmpBuffer, CURRENT_PROCESS_HANDLE, tmpBuffer + 0x10, 0x10);
	if (ret != 0) {
		printf("copyRemoteMemory failed: %08x\n", ret);
		return RESULT_ERROR;
	}
	rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), tmpBuffer, TMPBUFFER_SIZE);
	showMsgPaused("copyRemoteMemory/controlProcessMemory OK");
	


	bnInitParamsByHomeMenu();
	if (validateHomeMenuParams() != 0) {
		// Home menu Params is not complete, should be considered as unsupported
		ntrConfig->HomeMenuVersion = 0;
	}
	if (ntrConfig->HomeMenuVersion == 0) {
		showMsgPaused("unknown Home Menu");
	} 
	
	
	// load and execute ntr.bin, if firmware/home menu is unsupported, it will start ram dumping
	ret = bnLoadAndExecuteNTR();
	
	if ((ntrConfig->HomeMenuVersion == 0)  || (ntrConfig->firmVersion == 0)) {
		return RESULT_ERROR;
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


	printf("BootNTR 2.4\n");
	ntrConfig = &g_ntrConfig;
	bnConfig = &g_bnConfig;
	ret = bnBootNTR();
	if (ret == 0) {
		printf("NTR CFW loaded successfully\nExiting...\n(Hold SELECT to prevent auto-exit) \n");
		svcSleepThread(1000000000);
		isSuccess = 1;
	} else {
		printf("bnBootNTR failed, press START to exit.\n");
	}

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_SELECT) {
			isSuccess = 0;
		}
		if (isSuccess) {
			break;
		}
		if (kDown & KEY_START) {
			break; // break in order to return to hbmenu
		}
		
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}


	gfxExit();
	return 0;
}