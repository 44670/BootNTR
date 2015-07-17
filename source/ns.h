#define NS_DEFAULT_MEM_REGION 0x300

#define DEBUG_BUFFER_SIZE 0x4000
#define GLOBAL_BUFFER_SIZE 0x4000
#define MAX_BREAKPOINT 64

typedef struct _RT_LOCK {
	vu32 value;
} RT_LOCK;


#define NS_CONFIGURE_ADDR	0x06000000

typedef struct _RT_HOOK {
	u32 model;
	u32 isEnabled;
	u32 funcAddr;
	u32 bakCode[16];
	u32 jmpCode[16];
	u32 callCode[16];
} RT_HOOK;

typedef struct _NS_BREAKPOINT {
	u32 type;
	u32 flag;
	u32 addr;
	RT_HOOK hook;
	u32 stubCode[32];
	u32 isEnabled;
} NS_BREAKPOINT;



typedef struct _NS_CONFIG {
	u32 initMode;
	u32 startupCommand;
	u32 hSOCU;

	u8* debugBuf; 
	u32 debugBufSize;
	u32 debugPtr;
	u32 debugReady;

	RT_LOCK debugBufferLock;

	u32 startupInfo[32];
	u32 allowDirectScreenAccess;
	u32 exitFlag;

	u32 sharedFunc[100];

} NS_CONFIG;



void nsDbgPrint (const char*	fmt,	...	);

void rtInitLock(RT_LOCK *lock) ;
void rtAcquireLock(RT_LOCK *lock) ;
void rtReleaseLock(RT_LOCK *lock) ;
u32 rtAlignToPageSize(u32 size);
u32 rtGetPageOfAddress(u32 addr) ;
u32 rtCheckRemoteMemoryRegionSafeForWrite(Handle hProcess, u32 addr, u32 size) ;
u32 rtSafeCopyMemory(u32 dst, u32 src, u32 size) ;
int rtRecvSocket(u32 sockfd, u8 *buf, int size);
int rtSendSocket(u32 sockfd, u8 *buf, int size);
u16 rtIntToPortNumber(u16 x) ;
u32 rtGetFileSize(u8* fileName);
u32 rtLoadFileToBuffer(u8* fileName, u32* pBuf, u32 bufSize) ;
u32 rtGetThreadReg(Handle hProcess, u32 tid, u32* ctx);
u32 rtFlushInstructionCache(void* ptr, u32 size);
void rtInitHook(RT_HOOK* hook, u32 funcAddr, u32 callbackAddr);
void rtEnableHook(RT_HOOK* hook);
void rtDisableHook(RT_HOOK* hook);
u32 rtGenerateJumpCode(u32 dst, u32* buf);


u32 nsAttachProcess(Handle hProcess, u32 remotePC, NS_CONFIG *cfg) ;




