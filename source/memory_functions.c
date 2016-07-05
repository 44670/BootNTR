#include "main.h"

extern char			*g_primary_error;
extern char			*g_secondary_error;
extern NTR_CONFIG	g_ntrConfig;

u32		protectRemoteMemory(Handle hProcess, void *addr, u32 size)
{
	return (svc_controlProcessMemory(hProcess, addr, addr, size, 6, 7));
}

u32		copyRemoteMemory(Handle hDst, void *ptrDst, Handle hSrc, void *ptrSrc, u32 size)
{
	u32		dmaConfig[20] = { 0 };
	u32		hdma = 0;
	u32		state;
	u32		i;
	u32		result;

	if ((result = svc_flushProcessDataCache(hSrc, (u32)ptrSrc, size)) != 0)
		goto error;
	if ((result = svc_flushProcessDataCache(hDst, (u32)ptrDst, size)) != 0)
		goto error;
	if ((result = svc_startInterProcessDma(&hdma, hDst, ptrDst, hSrc, ptrSrc, size, dmaConfig)) != 0)
		goto error;
	state = 0;
	if (g_ntrConfig.InterProcessDmaFinishState == 0)
	{
		while (1)
		{
			svc_getDmaState(&state, hdma);
			svc_sleepThread(10000);
			result = state;
			svc_getDmaState(&state, hdma);
			if (result == state)
				break;
		}
		g_ntrConfig.InterProcessDmaFinishState = state;
	}
	else
	{
		for (i = 0; i < 1000000; i++)
		{			
			result = svc_getDmaState(&state, hdma);
			if (state == g_ntrConfig.InterProcessDmaFinishState)
				break;
			svc_sleepThread(10000);
		}
		if (i >= 1000000)
		{
			g_primary_error = READREMOTEMEMORY_TIMEOUT;
			svc_closeHandle(hdma);
			goto error;
		}
	}
	svc_closeHandle(hdma);
	if ((result = svc_invalidateProcessDataCache(hDst, (u32)ptrDst, size)) != 0)
		goto error;
	return (0);
error:
	return (RESULT_ERROR);
}

u32		patchRemoteProcess(u32 pid, u32 addr, u8 *buf, u32 len)
{
	u32		hProcess;
	u32		ret;

	ret = svc_openProcess(&hProcess, pid);
	check_prim(ret, OPENPROCESS_FAILURE);
	ret = protectRemoteMemory(hProcess, (void *)((addr / 0x1000) * 0x1000), 0x1000);
	check_prim(ret, PROTECTMEMORY_FAILURE);
	ret = copyRemoteMemory(hProcess, (void *)addr, 0xffff8001, buf, len);
	check_sec(ret, REMOTECOPY_FAILURE);
	if (hProcess)
		svc_closeHandle(hProcess);
	return (ret);
error:
	return (RESULT_ERROR);
}

u32		rtAlignToPageSize(u32 size)
{
	return (((size / 0x1000) + 1) * 0x1000);
}

u32		rtGetPageOfAddress(u32 addr)
{
	return ((addr / 0x1000) * 0x1000);
}


u32		rtCheckRemoteMemoryRegionSafeForWrite(Handle hProcess, u32 addr, u32 size)
{
	u32		ret;
	u32		startPage;
	u32		endPage;
	u32		page;

	startPage = rtGetPageOfAddress(addr);
	endPage = rtGetPageOfAddress(addr + size - 1);
	for (page = startPage; page <= endPage; page += 0x1000)
	{
		ret = protectRemoteMemory(hProcess, (void *)page, 0x1000);
		if (ret != 0)
			return (ret);
	}
	return (0);
}