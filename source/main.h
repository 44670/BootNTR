#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "ntr_config.h"
#include "mysvcs.h"
//printf("param is missing: %s\n", #a);
#define VALIDATE_PARAM(a) if ((a) == 0) { return (RESULT_ERROR); }
#define check_prim(result, err) if ((result) != 0) {g_primary_error = err; \
	goto error; }
#define check_sec(result, err) if ((result) != 0) {g_secondary_error = err; \
	goto error; }
#define check_third(result, err) if ((result) != 0) {g_third_error = err; \
	goto error; }


#define CURRENT_PROCESS_HANDLE	(0xffff8001)
#define RESULT_ERROR			(1)
#define TMPBUFFER_SIZE			(0x20000)

#define READREMOTEMEMORY_TIMEOUT	(char *)s_error[0]
#define OPENPROCESS_FAILURE			(char *)s_error[1]
#define PROTECTMEMORY_FAILURE		(char *)s_error[2]
#define REMOTECOPY_FAILURE			(char *)s_error[3]
#define CFGU_INIT_FAILURE			(char *)s_error[4]
#define CFGU_SECURE_FAILURE			(char *)s_error[5]
#define	WRONGREGION					(char *)s_error[6]
#define SMPATCH_FAILURE				(char *)s_error[7]
#define FSPATCH_FAILURE				(char *)s_error[8]
#define FILEOPEN_FAILURE			(char *)s_error[9]
#define NULLSIZE					(char *)s_error[10]
#define LINEARMEMALIGN_FAILURE		(char *)s_error[11]
#define ACCESSPATCH_FAILURE			(char *)s_error[12]
#define UNKNOWN_FIRM				(char *)s_error[13]
#define UNKNOWN_HOMEMENU			(char *)s_error[14]
#define USER_ABORT					(char *)s_error[15]
#define FILE_COPY_ERROR				(char *)s_error[16]
#define LOAD_FAILED					(char *)s_error[17]

static const char * const s_error[] =
{
	"READREMOTEMEMORY_TIMEOUT",
	"OPENPROCESS_FAILURE",
	"PROTECTMEMORY_FAILURE",
	"REMOTECOPY_FAILURE",
	"CFGU_INIT_FAILURE",
	"CFGU_SECURE_FAILURE",
	"WRONGREGION",
	"SMPATCH_FAILURE",
	"FSPATCH_FAILURE",
	"FILEOPEN_FAILURE",
	"NULLSIZE",
	"LINEARMEMALIGN_FAILURE",
	"ACCESSPATCH_FAILURE",
	"UNKNOWN_FIRM",
	"UNKNOWN_HOMEMENU",
	"USER_ABORT",
	"FILE_COPY_ERROR",
	"LOAD_AND_EXECUTE"

};

typedef void(*funcType)();
typedef struct	s_BLOCK
{
	u32			buffer[4];
}				t_BLOCK;

/*
** misc.s
*/
void	kFlushDataCache(void *, u32);
void	flushDataCache(void);
void	FlushAllCache(void);
void	InvalidateEntireInstructionCache(void);
void	InvalidateEntireDataCache(void);
void	backdoorHandler();

/*
** main.c
*/
void	printMenu(int update);

/*
** files.c
*/
int		copy_file(char *old_filename, char  *new_filename);

/*
** common_functions.c
*/
bool	abort_and_exit(void);
u32		getCurrentProcessHandle(void);
void	flushDataCache(void);
void	doFlushCache(void);
void	doStallCpu(void);
void	doWait(void);

/*
** ntr_launcher.c
*/
Result		bnPatchAccessCheck(void);
Result		bnLoadAndExecuteNTR(void);
Result		bnBootNTR(void);

/*
** memory_functions.c
*/
u32		protectRemoteMemory(Handle hProcess, void *addr, u32 size);
u32		copyRemoteMemory(Handle hDst, void *ptrDst, Handle hSrc, void *ptrSrc, u32 size);
u32		patchRemoteProcess(u32 pid, u32 addr, u8 *buf, u32 len);
u32		rtAlignToPageSize(u32 size);
u32		rtGetPageOfAddress(u32 addr);
u32		rtCheckRemoteMemoryRegionSafeForWrite(Handle hProcess, u32 addr, u32 size);

/*
** firmware.c
*/
void	bnInitParamsByFirmware(void);
Result	validateFirmParams(void);

/*
** kernel.c
*/
void	kernelCallback(void);

/*
** homemenu.c
*/
Result	bnInitParamsByHomeMenu(void);
Result	validateHomeMenuParams(void);

#endif