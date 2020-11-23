#include "main.h"
#include "config.h"
#include "csvc.h"

extern char         *g_primary_error;
extern char         *g_secondary_error;
extern ntrConfig_t  *ntrConfig;

#define LOCAL_MAP_ADDR_SRC 0x24000000
#define LOCAL_MAP_ADDR_DST 0x25000000

u32     protectRemoteMemory(Handle hProcess, u32 addr, u32 size)
{
    return (svcControlProcessMemory(hProcess, addr, addr, size, 6, 7));
}

u32     copyRemoteMemory(Handle hDst, u32 ptrDst, Handle hSrc, u32 ptrSrc, u32 size)
{
    bool isPlgLoader = isPluginLoaderLuma();
    ntrConfig->InterProcessDmaFinishState = DMASTATE_STARTING;
    Result res = 0;
    Handle tmpHandle;
    u32 pageSrc = ptrSrc & ~0xFFF, pageDst = ptrDst & ~0xFFF;
    u32 offsetSrc = ptrSrc - pageSrc, offsetDst = ptrDst - pageDst;
    u32 pageSize = ((size + ((offsetSrc > offsetDst) ? offsetSrc : offsetDst)) & ~0xFFF) + 0x1000;

    if (isPlgLoader) res = svcMapProcessMemoryExPluginLoader(CUR_PROCESS_HANDLE, LOCAL_MAP_ADDR_SRC, hSrc, pageSrc, pageSize);
    else res = svcMapProcessMemoryEx(hSrc, LOCAL_MAP_ADDR_SRC, pageSrc, pageSize);

    if (R_FAILED(res))
        return RESULT_ERROR;

    if (isPlgLoader) res = svcMapProcessMemoryExPluginLoader(CUR_PROCESS_HANDLE, LOCAL_MAP_ADDR_DST, hDst, pageDst, pageSize);
    else res = svcMapProcessMemoryEx(hDst, LOCAL_MAP_ADDR_DST, pageDst, pageSize);

    if (R_FAILED(res)) {
        tmpHandle = isPlgLoader ? CUR_PROCESS_HANDLE : hSrc;
        svcUnmapProcessMemoryEx(tmpHandle, LOCAL_MAP_ADDR_SRC, pageSize);
        return RESULT_ERROR;
    }
    ntrConfig->InterProcessDmaFinishState = DMASTATE_RUNNING;

    u32 currID = 0, remoteID = 0;
    svcGetProcessId(&currID, hDst);
    svcGetProcessId(&remoteID, CURRENT_PROCESS_HANDLE);

    if (isPlgLoader && currID != remoteID) svcControlProcess(hDst, PROCESSOP_SCHEDULE_THREADS, 1, 0); // More stable in 3GX Loader luma builds
    memcpy((u8*)(LOCAL_MAP_ADDR_DST + offsetDst), (u8*)(LOCAL_MAP_ADDR_SRC + offsetSrc), size);

    tmpHandle = isPlgLoader ? CUR_PROCESS_HANDLE : hDst;
    svcUnmapProcessMemoryEx(tmpHandle, LOCAL_MAP_ADDR_DST, pageSize);

    svcInvalidateProcessDataCache(hDst, (u32)ptrDst, size);
    svcInvalidateEntireInstructionCache();
    if (isPlgLoader && currID != remoteID) svcControlProcess(hDst, PROCESSOP_SCHEDULE_THREADS, 0, 0);

    tmpHandle = isPlgLoader ? CUR_PROCESS_HANDLE : hSrc;
    svcUnmapProcessMemoryEx(tmpHandle, LOCAL_MAP_ADDR_SRC, pageSize);
    
    svcSleepThread(10 * 1000 * 1000);
    ntrConfig->InterProcessDmaFinishState = DMASTATE_DONE;
    return 0;
}

u32     patchRemoteProcess(u32 pid, u32 addr, u8 *buf, u32 len)
{
    if (!addr || !buf) return 0;

    u32     hProcess;
    u32     ret;

    ret = svc_openProcess(&hProcess, pid);
    check_prim(ret, OPENPROCESS_FAILURE);
    ret = protectRemoteMemory(hProcess, ((addr / 0x1000) * 0x1000), 0x1000);
    check_prim(ret, PROTECTMEMORY_FAILURE);
    ret = copyRemoteMemory(hProcess, addr, CURRENT_PROCESS_HANDLE, (u32)buf, len);
    check_sec(ret, REMOTECOPY_FAILURE);
    if (hProcess)
        svcCloseHandle(hProcess);
    return (ret);
error:
    return (RESULT_ERROR);
}

u32     rtAlignToPageSize(u32 size)
{
    return (((size / 0x1000) + 1) * 0x1000);
}

u32     rtGetPageOfAddress(u32 addr)
{
    return ((addr / 0x1000) * 0x1000);
}

u32     rtCheckRemoteMemoryRegionSafeForWrite(Handle hProcess, u32 addr, u32 size)
{
    u32     ret;
    u32     startPage;
    u32     endPage;
    u32     page;

    startPage = rtGetPageOfAddress(addr);
    endPage = rtGetPageOfAddress(addr + size - 1);
    for (page = startPage; page <= endPage; page += 0x1000)
    {
        ret = protectRemoteMemory(hProcess, page, 0x1000);
        if (ret != 0)
            return (ret);
    }
    return (0);
}

#define ALPHABET_LEN 256

// Quick Search algorithm, adapted from
// http://igm.univ-mlv.fr/~lecroq/string/node19.html#SECTION00190
u32     memfind(u8 *startPos, u32 size, const void *pattern, u32 patternSize)
{
    u32     i;
    u32     j;
    u32     table[ALPHABET_LEN];

    const u8 *patternc = (const u8 *)pattern;

    // Preprocessing
    for (i = 0; i < ALPHABET_LEN; ++i)
        table[i] = patternSize + 1;
    for (i = 0; i < patternSize; ++i)
        table[patternc[i]] = patternSize - i;

    // Searching
    j = 0;
    while (j <= size - patternSize)
    {
        if (memcmp(patternc, startPos + j, patternSize) == 0)
            return (j);
        j += table[startPos[j + patternSize]];
    }
    return (0);
}

u32     findNearestSTMFD(u32 base, u32 pos) 
{
    if (pos < base)
    {
        return 0;
    }
    pos = pos - pos % 4;
    u32 term = pos - 0x1000;
    if (term < base)
    {
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

u32     searchBytes(u32 startAddr, u32 endAddr, u8* pat, int patlen, int step)
{
    u32 pat0 = ((u32*)pat)[0];

    while (1)
    {
        if (startAddr + patlen >= endAddr)
        {
                return 0;
        }
        if (*((u32*)(startAddr)) == pat0)
        {
            if (memcmp((u32*) startAddr, pat, patlen) == 0)
            {
                return startAddr;
            }
        }
        startAddr += step;
    }
    return 0;
}

