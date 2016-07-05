#include "main.h"

extern NTR_CONFIG		g_ntrConfig;
extern BOOTNTR_CONFIG	g_bnConfig;
extern NTR_CONFIG		*ntrConfig;
extern BOOTNTR_CONFIG	*bnConfig;
extern u8				*tmpBuffer;
extern char				*g_primary_error;
extern char				*g_secondary_error;
extern char				*g_third_error;

Result		bnPatchAccessCheck(void)
{
	Result	ret;
	u8		smPatchBuf[] = { 0x02, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1 };
	u32		fsPatchValue = 0x47702001;

	check_prim(abort_and_exit(), USER_ABORT);
	svc_backDoor((void*)backdoorHandler);
	// do a dma copy to get the finish state value on current console
	ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, tmpBuffer, CURRENT_PROCESS_HANDLE, tmpBuffer + 0x10, 0x10);
	check_sec(ret, REMOTECOPY_FAILURE);
	ret = patchRemoteProcess(bnConfig->SMPid, bnConfig->SMPatchAddr, smPatchBuf, 8);
	check_sec(ret, SMPATCH_FAILURE);
	ret = patchRemoteProcess(bnConfig->FSPid, bnConfig->FSPatchAddr, (u8 *)&fsPatchValue, 4);
	check_sec(ret, FSPATCH_FAILURE);
	return (0);
error:
	return (RESULT_ERROR);
}

Result		bnLoadAndExecuteNTR(void)
{
	u32		size;
	u32		outAddr;
	u32		totalSize;
	u32		*bootArgs;
	Handle	fsUserHandle;
	FILE	*file;

	check_prim(abort_and_exit(), USER_ABORT);
	file = fopen("sdmc:/ntr.bin", "rb");
	check_prim(!file, FILEOPEN_FAILURE);
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	check_prim(!size, NULLSIZE);
	fseek(file, 0, SEEK_SET);
	ntrConfig->arm11BinSize = rtAlignToPageSize(size);
	totalSize = (ntrConfig->arm11BinSize) * 2;
	outAddr = (u32)linearMemAlign(totalSize, 0x1000);
	check_prim(!outAddr, LINEARMEMALIGN_FAILURE);
	ntrConfig->arm11BinStart = (outAddr + ntrConfig->arm11BinSize);
	rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), outAddr, totalSize);
	memset((void *)outAddr, 0, totalSize);
	fread((void *)outAddr, size, 1, file);
	memcpy((void *)(outAddr + (ntrConfig->arm11BinSize)), (void *)outAddr, size);
	fsUserHandle = 0;
	srvGetServiceHandle(&fsUserHandle, "fs:USER");
	FSUSER_Initialize(fsUserHandle);
	ntrConfig->fsUserHandle = fsUserHandle;
	bootArgs = (u32 *)(outAddr + 4);
	bootArgs[0] = 0;
	bootArgs[1] = 0xb00d;
	bootArgs[2] = (u32)ntrConfig;
	((funcType)(outAddr))();
	svcCloseHandle(fsUserHandle);
	return (0);
error:
	return (RESULT_ERROR);
}

Result		bnBootNTR(void)
{
	Result ret;

	tmpBuffer = (u8 *)linearMemAlign(TMPBUFFER_SIZE, 0x1000);
	check_prim(!tmpBuffer, LINEARMEMALIGN_FAILURE);
	bnInitParamsByFirmware();
	if (validateFirmParams() != 0)
	{
		ntrConfig->firmVersion = 0;
		check_prim(RESULT_ERROR, UNKNOWN_FIRM);
	}
	ret = bnPatchAccessCheck();
	check_third(ret, ACCESSPATCH_FAILURE);
	ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, tmpBuffer, CURRENT_PROCESS_HANDLE, tmpBuffer + 0x10, 0x10);
	check_sec(ret, REMOTECOPY_FAILURE);
	rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), (u32)tmpBuffer, TMPBUFFER_SIZE);
	bnInitParamsByHomeMenu();
	if (validateHomeMenuParams() != 0)
	{
		// Home menu Params is not complete, should be considered as unsupported
		ntrConfig->HomeMenuVersion = 0;
		check_sec(RESULT_ERROR, UNKNOWN_HOMEMENU);
	}
	ret = bnLoadAndExecuteNTR();
	check_third(ret, LOAD_FAILED);
	return (ret);
error:
	return (RESULT_ERROR);
}
