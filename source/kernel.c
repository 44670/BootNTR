#include "main.h"
#include "config.h"

extern ntrConfig_t      *ntrConfig;
extern bootNtrConfig_t  *bnConfig;
extern u8               *tmpBuffer;
extern char             *g_error;

typedef struct
{
    void(*invalidateDataCache)(void *, u32);
    void(*storeDataCache)(void *, u32);
    void(*flushDataCache)(void *, u32);
    void(*flushInstructionCache)(void *, u32);
} dbgKernelCacheInterface;

dbgKernelCacheInterface cacheInterface_NEW81 =
{
    //for new 3ds 8.1
    (void*)0xFFF24C9C,
    (void*)0xFFF1CF7C,
    (void*)0xFFF1CCA0,
    (void*)0xFFF1F04C
};

dbgKernelCacheInterface cacheInterface_NEW92 =
{
    //for new 3ds 9.2
    (void*)0xFFF25768,
    (void*)0xFFF1D9D4,
    (void*)0xFFF1D67C,
    (void*)0xFFF1FEEC
};

dbgKernelCacheInterface cacheInterface_NEW95 =
{
    //for new 3ds 9.5
    (void*)0xFFF25BD8,
    (void*)0xFFF1D9AC,
    (void*)0xFFF1D654,
    (void*)0xFFF1FCE8
};

dbgKernelCacheInterface cacheInterface_NEW96 =
{
    //for new3ds 9.6
    (void*)0xFFF25C24,
    (void*)0xFFF1D9D4,
    (void*)0xFFF1D67C,
    (void*)0xFFF1FD10
};

dbgKernelCacheInterface cacheInterface_NEW102 =
{
    //for new3ds 10.2
    (void*)0xFFF25BFC,
    (void*)0xFFF1D9AC,
    (void*)0xFFF1D654,
    (void*)0xFFF1FCE8
};

dbgKernelCacheInterface cacheInterface_NEW110 =
{
    //for new 3ds 11.0
    (void*)0xFFF26174,
    (void*)0xFFF1DEF0,
    (void*)0xFFF1DB98,
    (void*)0xFFF2022C
};

dbgKernelCacheInterface cacheInterface_NEW111 =
{
    //for new 3ds 11.1
    (void*)0xFFF261F0,
    (void*)0xFFF1DF6C,
    (void*)0xFFF1DC14,
    (void*)0xFFF202A8
};

dbgKernelCacheInterface cacheInterface_NEW112 =
{
    //for new 3ds 11.2
    (void*)0xFFF26210, 
    (void*)0xFFF1DF8C,
    (void*)0xFFF1DC34,
    (void*)0xFFF202C8
};

dbgKernelCacheInterface cacheInterface_NEW113 = 
{
    //for new 3ds 11.3
    (void*)0xFFF27400,
    (void*)0xFFF1E15C,
    (void*)0xFFF1DE04,
    (void*)0xFFF20498
};

dbgKernelCacheInterface cacheInterface_NEW114 =
{
    //for new 3ds 11.4
    (void*)0xFFF27480,
    (void*)0xFFF1E1DC,
    (void*)0xFFF1DE84,
    (void*)0xFFF20518
};

dbgKernelCacheInterface cacheInterface_NEW118 =
{
	//for new 3ds 11.8
	(void*)0xFFF27480,
	(void*)0xFFF1E1DC,
	(void*)0xFFF1DE84,
	(void*)0xFFF20518
};

dbgKernelCacheInterface cacheInterface_NEW1114 = {
    //for new 3ds 11.14
    (void*)0xFFF2746C,
    (void*)0XFFF1E1C8,
    (void*)0XFFF1DE70,
    (void*)0xFFF20504
};

dbgKernelCacheInterface cacheInterface_Old90 =
{
    //for old 3ds 9.0
    (void*)0xFFF24B54,
    (void*)0xFFF1CC5C,
    (void*)0xFFF1C9F4,
    (void*)0xFFF1F47C
};

dbgKernelCacheInterface cacheInterface_Old96 =
{
    //for old 3ds 9.6
    (void*)0xFFF24FF0,
    (void*)0xFFF1CF98,
    (void*)0xFFF1CD30,
    (void*)0xFFF1F748
};

dbgKernelCacheInterface cacheInterface_Old110 =
{
    //for old 3ds 11.0
    (void*)0xFFF2552C,
    (void*)0xFFF1D758,
    (void*)0xFFF1D4F0,
    (void*)0xFFF1FC50
};

dbgKernelCacheInterface cacheInterface_Old111 =
{
    //for old 3ds 11.1
    (void*)0xFFF255A8,
    (void*)0xFFF1D7D4,
    (void*)0xFFF1D56C,
    (void*)0xFFF1FCCC
};

dbgKernelCacheInterface cacheInterface_Old112 =
{
    //for old 3ds 11.2
    (void*)0xFFF255C8,
    (void*)0xFFF1D7F4,
    (void*)0xFFF1D58C,
    (void*)0xFFF1FCEC
};

dbgKernelCacheInterface cacheInterface_Old113 = 
{
    //for old 3ds 11.3
    (void*)0xFFF257D0,
    (void*)0xFFF1D9DC,
    (void*)0xFFF1D774,
    (void*)0xFFF1FED4
};

dbgKernelCacheInterface cacheInterface_Old114 =
{
    //for old 3ds 11.4
    (void*)0xFFF25850,
    (void*)0xFFF1DA5C,
    (void*)0xFFF1D7F4,
    (void*)0xFFF1FF54
};

dbgKernelCacheInterface cacheInterface_Old118 =
{
	//for old 3ds 11.8
	(void*)0xFFF257B4,
	(void*)0xFFF1D9C0,
	(void*)0xFFF1D758,
	(void*)0xFFF1FEB8
};

dbgKernelCacheInterface cacheInterface_Old1114 = {
    //for old 3ds 11.14
    (void*)0xFFF25830,
    (void*)0XFFF1DA3C,
    (void*)0XFFF1D7D4,
    (void*)0xFFF1FF34
};

void    kernelCallback(void)
{
    u32                         svc_patch_addr = bnConfig->SvcPatchAddr;
    u32                         firmVersion = ntrConfig->firmVersion;
    u32                         isNew3DS = ntrConfig->isNew3DS;
    dbgKernelCacheInterface     *cache = NULL;

    if (isNew3DS)
    {
        if (firmVersion == SYSTEM_VERSION(8, 1, 0))
            cache = &cacheInterface_NEW81;
        else if (firmVersion == SYSTEM_VERSION(9, 2, 0))
            cache = &cacheInterface_NEW92;
        else if (firmVersion == SYSTEM_VERSION(9, 5, 0))
            cache = &cacheInterface_NEW95;
        else if (firmVersion == SYSTEM_VERSION(9, 6, 0))
            cache = &cacheInterface_NEW96;
        else if (firmVersion == SYSTEM_VERSION(10, 2, 0))
            cache = &cacheInterface_NEW102;
        else if (firmVersion == SYSTEM_VERSION(11, 0, 0))
            cache = &cacheInterface_NEW110;
        else if (firmVersion == SYSTEM_VERSION(11, 1, 0))
            cache = &cacheInterface_NEW111;
        else if (firmVersion == SYSTEM_VERSION(11, 2, 0))
            cache = &cacheInterface_NEW112;
        else if (firmVersion == SYSTEM_VERSION(11, 3, 0))
            cache = &cacheInterface_NEW113;
        else if (firmVersion == SYSTEM_VERSION(11, 4, 0))
            cache = &cacheInterface_NEW114;
		else if (firmVersion == SYSTEM_VERSION(11, 8, 0))
			cache = &cacheInterface_NEW118;
        else if (firmVersion == SYSTEM_VERSION(11, 14, 0))
            cache = &cacheInterface_NEW1114;
    }
    else
    {
        if (firmVersion == SYSTEM_VERSION(9, 0, 0))
            cache = &cacheInterface_Old90;
        else if (firmVersion == SYSTEM_VERSION(9, 6, 0))
            cache = &cacheInterface_Old96;
        else if (firmVersion == SYSTEM_VERSION(11, 0, 0))
            cache = &cacheInterface_Old110;
        else if (firmVersion == SYSTEM_VERSION(11, 1, 0))
            cache = &cacheInterface_Old111;
        else if (firmVersion == SYSTEM_VERSION(11, 2, 0))
            cache = &cacheInterface_Old112;
        else if (firmVersion == SYSTEM_VERSION(11, 3, 0))
            cache = &cacheInterface_Old113;
        else if (firmVersion == SYSTEM_VERSION(11, 4, 0))
            cache = &cacheInterface_Old114;
		else if (firmVersion == SYSTEM_VERSION(11, 8, 0))
			cache = &cacheInterface_Old118;
        else if (firmVersion == SYSTEM_VERSION(11, 14, 0))
            cache = &cacheInterface_Old1114;
    }
    *(int *)(svc_patch_addr + 8) = 0xE1A00000; //NOP
    *(int *)(svc_patch_addr) = 0xE1A00000; //NOP
    kFlushDataCache((void *)svc_patch_addr, 0x10);//
    if (cache)
    {
        cache->invalidateDataCache((void *)svc_patch_addr, 0x10);//
        cache->flushInstructionCache((void *)(svc_patch_addr - 0xDFF80000 + 0xFFF00000), 0x10);//
    }
}
