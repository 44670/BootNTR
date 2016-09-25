#include "main.h"
#include "graphics.h"

extern u8       *tmpBuffer;
extern char     *g_primary_error;
extern char     *g_secondary_error;
extern bool     g_exit;

bool	abort_and_exit(void)
{
	hidScanInput();
	if ((hidKeysDown() | hidKeysHeld() | hidKeysUp()) & KEY_B)
	{
		g_exit = true;
		g_primary_error = USER_ABORT;
        newAppInfoEntry(DEFAULT_COLOR, CENTER | SKINNY | SMALL, "Loading aborted");
		while (1)
		{
			hidScanInput();
			if ((hidKeysDown() | hidKeysUp() | hidKeysHeld()) == 0)
				break;
		}
		return (true);
	}
	return (false);
}

u32		getCurrentProcessHandle(void)
{
	u32 handle = 0;
	u32 ret;
	u32 hCurrentProcess;
	u32 currentPid;

	svc_getProcessId(&currentPid, 0xffff8001);
	ret = svc_openProcess(&handle, currentPid);
	if (ret != 0)
		return (0);
	hCurrentProcess = handle;
	return (hCurrentProcess);
}

void	flushDataCache(void)
{
	u32	*ptr = (u32 *)tmpBuffer;
	u32 i;

	for (i = 0; i < (0x20000 / 4); i++)
	{
		ptr[i] += 0x11111111;
	}
}

void	doFlushCache(void)
{
	FlushAllCache();
	InvalidateEntireInstructionCache();
	InvalidateEntireDataCache();
	flushDataCache();
	FlushAllCache();
	InvalidateEntireInstructionCache();
	InvalidateEntireDataCache();
}

void doStallCpu(void)
{
	vu32 i;
	for (i = 0; i < 0x00100000; i++)
	{
	}
}

void	doWait(void)
{
	svcSleepThread(500000000);
}

