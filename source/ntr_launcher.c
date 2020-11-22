#include "main.h"
#include "config.h"
#include "csvc.h"

extern ntrConfig_t      *ntrConfig;
extern bootNtrConfig_t  *bnConfig;
extern u8               *tmpBuffer;
extern bool              g_exit;
extern char             *g_primary_error;
extern char             *g_secondary_error;
extern char             *g_third_error;

int         isNTRAlreadyLaunched(void)
{
    Result  ret;
    Handle  processHandle;

    ret = svcOpenProcess(&processHandle, 0xf);
    if (ret) goto error;
    ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, (u32)tmpBuffer, processHandle, 0x06000000, 0x1000);
    svcCloseHandle(processHandle);
    if (!ret)
    {
        g_exit = true;
        goto error;
    }
    return (0);
error:
    return (RESULT_ERROR);
}

bool        isPluginLoaderLuma()
{
    static u32 state = 0;
    if (state == 0) {
        Handle tmpHandle;
        Result res = svcConnectToPort(&tmpHandle, "plg:ldr");
        if (R_SUCCEEDED(res)) {
            svcCloseHandle(tmpHandle);
            state = 2;
        }
        else state = 1;
    }
	return state == 2;
}

u32			findCustomPMsvcRunPattern(u32* outaddr)
{
	Handle prochand;
	bool isPlgLoader = isPluginLoaderLuma();
	u32 textStart = 0;
	*outaddr = 0;
	u32 res = 0;
	res = svcOpenProcess(&prochand, ntrConfig->PMPid); //pm processID
	if (res)
	{
		return res;
	}
	s64 info;
	res = svcGetProcessInfo(&info, prochand, 0x10005); //get start of .text
	if (res)
	{
		return res;
	}
	u32* addr = (u32*)(u32)info;
	textStart = info;
	res = svcGetProcessInfo(&info, prochand, 0x10002); //get .text size
	if (res)
	{
		return res;
	}
	if (isPlgLoader) res = svcMapProcessMemoryExPluginLoader(CURRENT_PROCESS_HANDLE, 0x28000000, prochand, (u32)addr, (u32)info);
	else res = svcMapProcessMemoryEx(prochand, 0x28000000, (u32)addr, (u32)info); //map PM process memory into this process @ 0x08000000
	if (res)
	{
		return res;
	}
	addr = (u32*)0x28000000;
	u32* endAddr = (u32*)((u32)addr + (u32)info);
	while (addr < endAddr)
	{
		if (*addr == 0xEF000012) // Find svc 0x12
		{
			u32* addr2 = addr;
			while (*addr2 != 0xE92D0030 && (u32)addr - (u32)addr2 < 0x100) addr2--; // Find start of the svc function
			if ((u32)addr - (u32)addr2 < 0x100)
			{
				*outaddr = ((u32)addr2 - 0x28000000) + textStart;
			}
			else res = -1;
			break;
		}
		addr++;
	}
	if (addr >= endAddr) res = -1;
	svcUnmapProcessMemoryEx(CURRENT_PROCESS_HANDLE, 0x28000000, (u32)info);
	svcCloseHandle(prochand);
	return res;
}

Result		bnPatchCustomPM() { // If cfw is Luma3DS 10 and higher, patch custom PM sysmodule
	Result ret = 0;
	s64 out;
	if (R_SUCCEEDED(ret = svcGetSystemInfo(&out, 0x10000, 0))
		&& GET_VERSION_MAJOR((u32)out) >= 10)
	{
		u32 patchAddr;
		check_prim(ret = findCustomPMsvcRunPattern(&patchAddr), CUSTOM_PM_PATCH_FAIL);
		ntrConfig->PMSvcRunAddr = patchAddr;
	}
	else ret = 0;
error:
	return ret;
}

Result      bnPatchAccessCheck(void)
{
    s64     out;
    Result  ret = 0;
    u8      smPatchBuf[] = { 0x02, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1 };
    u32     fsPatchValue = 0x47702001;

    // Patch firm
    svcBackdoor(backdoorHandler);

    // Do a dma copy to get the finish state value on current console
    ret = copyRemoteMemory(CURRENT_PROCESS_HANDLE, (u32)tmpBuffer, CURRENT_PROCESS_HANDLE, (u32)tmpBuffer + 0x10, 0x10);
    check_sec(ret, REMOTECOPY_FAILURE);

    ret = patchRemoteProcess(bnConfig->FSPid, bnConfig->FSPatchAddr, (u8 *)&fsPatchValue, 4);
    check_sec(ret, FSPATCH_FAILURE);

    // If cfw is Luma3DS 9 and higher, skip sm patch
    if (R_SUCCEEDED(ret = svcGetSystemInfo(&out, 0x10000, 0)) && GET_VERSION_MAJOR((u32)out) >= 9)
        return (0);
	
    ret = patchRemoteProcess(bnConfig->SMPid, bnConfig->SMPatchAddr, smPatchBuf, 8);
    check_sec(ret, SMPATCH_FAILURE);
    return (0);
error:
    return (RESULT_ERROR);
}

u32 loadNTRBin(void)
{
    u32                 size;
    u32                 alignedSize;
    u8                  *mem;
    FILE                *ntr;
    u32                 ret;
    char                path[0x100];


    extern const char   *outNtrVersionStrings[3];

    if (bnConfig->versionToLaunch == V32)
        strJoin(path, "/", "ntr.bin");
    else
        strJoin(path, bnConfig->config->binariesPath + 5, outNtrVersionStrings[bnConfig->versionToLaunch]);

    if (bnConfig->versionToLaunch == V32)
        strcpy(ntrConfig->path, path);
    if (bnConfig->versionToLaunch == V36)
    {
        strcpy(ntrConfig->path, path);
    #if EXTENDEDMODE
        ntrConfig->memorymode = 3;
    #else
        ntrConfig->memorymode = 0;
    #endif
    }
    
    // Get size
    ntr = fopen(path, "rb");
    check_prim(!ntr, FILEOPEN_FAILURE);
    fseek(ntr, 0, SEEK_END);
    size = ftell(ntr);
    fseek(ntr, 0, SEEK_SET);
    alignedSize = rtAlignToPageSize(size);
    ntrConfig->arm11BinSize = alignedSize;

    // Allocate memory
    mem = (u8 *)linearMemAlign(alignedSize * 2, 0x1000);
    check_sec(!mem, LINEARMEMALIGN_FAILURE);
    ntrConfig->arm11BinStart = ((u32)mem + alignedSize);
    ret = rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), (u32)mem, alignedSize * 2);
    check_prim(ret, PROTECTMEMORY_FAILURE);

    // Read to memory
    memset(mem, 0, alignedSize * 2);
    u8 *temp = (u8 *)calloc(1, alignedSize);
    fread(temp, size, 1, ntr);
    fclose(ntr);
    svcFlushProcessDataCache(getCurrentProcessHandle(), (u32)temp, alignedSize);

    svcInvalidateProcessDataCache(getCurrentProcessHandle(), (u32)mem, alignedSize * 2);
    memcpy(mem, temp, size);
    memcpy(mem + alignedSize, temp, size);
    svcFlushProcessDataCache(getCurrentProcessHandle(), (u32)mem, alignedSize *2);
    free(temp);
    return ((u32)mem);
error:
    return (RESULT_ERROR);
}

Result      bnLoadAndExecuteNTR(void)
{
    u32     outAddr;
    u32     *bootArgs;

    outAddr = loadNTRBin();
    if (outAddr == RESULT_ERROR) 
    {
            goto error;
    }
    bootArgs = (u32 *)(outAddr + 4);
    bootArgs[0] = 0;
    bootArgs[1] = 0xb00d;
    bootArgs[2] = (u32)ntrConfig;
    ((funcType)(outAddr))();
    return (0);
error:
    return (RESULT_ERROR);
}
void        showDbg(char *str)
{
     newAppTop(DEFAULT_COLOR, TINY | SKINNY, str);
}

Result      bnBootNTR(void)
{
    Result  ret;
    u8      *linearAddress;

    linearAddress = NULL;
    
    // Set firm params
    check_prim(bnInitParamsByFirmware(), UNKNOWN_FIRM);
    
    // Alloc temp buffer
    linearAddress = (u8 *)linearMemAlign(TMPBUFFER_SIZE, 0x1000);
    tmpBuffer = linearAddress;
    check_prim(!tmpBuffer, LINEARMEMALIGN_FAILURE);
    rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), (u32)tmpBuffer, TMPBUFFER_SIZE);
    ret = isNTRAlreadyLaunched();
    check_prim(ret, NTR_ALREADY_LAUNCHED);
    // Patch services
    ret = bnPatchAccessCheck();
	check_prim(ret, ACCESSPATCH_FAILURE);
	// Patch custom PM
	ret = bnPatchCustomPM();
	check_prim(ret, CUSTOM_PM_PATCH_FAIL);
    
    // Init home menu params
    check_sec(bnInitParamsByHomeMenu(), UNKNOWN_HOMEMENU);
    
    // Free temp buffer
    linearFree(linearAddress);

    // Show debug logs from NTR if X is hold
    hidScanInput();
    if (bnConfig->isDebug || (hidKeysDown() | hidKeysHeld()) & KEY_X)
        ntrConfig->ShowDbgFunc = (u32)showDbg;
    // Load NTR
    ret = bnLoadAndExecuteNTR();
    check_third(ret, LOAD_FAILED);
    return (ret);
error:
    if (linearAddress) linearFree(linearAddress);
    return (RESULT_ERROR);
}

static char *dumpString[] =
{
    "Kernel",
    "FS Process",
    "PM Process",
    "SM Process",
    "HomeMenu Process"
};

void        printDumpLog(char *str)
{
    static int  remove = 0;
    static int  changeMode = 0;
    static int  mode = 0;
    static char address[9] = "DFFF0000";
    //Mode:
    // 0 = Kernel Shared Memeory
    // 1 = FS
    // 2 = PM
    // 3 = SM
    // 4 = HomeMenu

    if (strncmp(str, "dump finished", 13) == 0)
    {
        changeMode = 1;
        str += 5;
    }
    if (changeMode || strncmp(str, "addr:", 5) == 0)
    {
        if (mode == 0)
        {
            strncpyFromTail(address, str, 8);
            if (*address == '0')
            {
                remove = 0;
                mode += 1;
            }
        }
        if (remove) removeAppTop();
        remove = 1;
        newAppTop(DEFAULT_COLOR, TINY | SKINNY, "%s\uE019%s", dumpString[mode], str);
        mode += changeMode;
        remove = !changeMode;
        changeMode = 0;
    }
    else
    {
        if (strncmp(str, "current", 7) && strncmp(str, "firmware", 8))
            newAppTop(DEFAULT_COLOR, TINY | SKINNY, str);
    }
}

void        launchNTRDumpMode(void)
{
    u32     isNew3DS = 0;
    char    buffer[0x20];

    APT_CheckNew3DS((bool *)&isNew3DS);

    ntrConfig->firmVersion = 0;
    ntrConfig->HomeMenuVersion = 0;
    ntrConfig->isNew3DS = isNew3DS;
    ntrConfig->PMPid = 2;
    ntrConfig->HomeMenuPid = 0xf;
    bnConfig->versionToLaunch = V33;
    ntrConfig->ShowDbgFunc = (u32)printDumpLog;
    copyRemoteMemory(CURRENT_PROCESS_HANDLE, (u32)buffer, CURRENT_PROCESS_HANDLE, (u32)buffer + 0x10, 0x10);
    bnLoadAndExecuteNTR();
}
