#ifndef CONFIG_H
#define CONFIG_H

#include "main.h"

enum
{
    LV32 = BIT(0),
    LV33 = BIT(1),
    LV34 = BIT(2),
    CUSTOM_PLUGIN_PATH = BIT(3)
}configFlags;

typedef struct  config_s
{
    u32         flags;
    char        binariesPath[0x100];
    char        pluginPath[0x100];
}               config_t;

typedef struct  ntrConfig_s
{
    u32         bootNTRVersion;
    u32         isNew3DS;
    u32         firmVersion;

    u32         IoBasePad;
    u32         IoBaseLcd;
    u32         IoBasePdc;
    u32         PMSvcRunAddr;
    u32         PMPid;
    u32         HomeMenuPid;

    u32         HomeMenuVersion;
    u32         HomeMenuInjectAddr; // FlushDataCache Function
    u32         HomeFSReadAddr;
    u32         HomeFSUHandleAddr;
    u32         HomeCardUpdateInitAddr;
    u32         HomeAptStartAppletAddr;

    u32         KProcessHandleDataOffset;
    u32         KProcessPIDOffset;
    u32         KProcessCodesetOffset;
    u32         ControlMemoryPatchAddr1;
    u32         ControlMemoryPatchAddr2;
    u32         KernelFreeSpaceAddr_Optional;
    u32         KMMUHaxAddr;
    u32         KMMUHaxSize;
    u32         InterProcessDmaFinishState;
    u32         fsUserHandle;
    u32         arm11BinStart;
    u32         arm11BinSize;
    u32         ShowDbgFunc;
}               ntrConfig_t;

typedef struct  bootNtrConfig_s
{
    u32         FSPatchAddr;
    u32         SMPatchAddr;
    u32         SvcPatchAddr;
    u32         FSPid;
    u32         SMPid;
    u32         requireKernelHax;
    version_t   versionToLaunch;
    config_t    *config;
}               bootNtrConfig_t;

void    configInit(void);
void    resetConfig(void);
void    configExit(void);
bool    checkPath(void);

void    firstLaunch(void);

#endif