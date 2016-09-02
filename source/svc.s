.arm

.align 4

.global getThreadCommandBuffer
.type getThreadCommandBuffer, %function
getThreadCommandBuffer:
	mrc p15, 0, r0, c13, c0, 3
	add r0, #0x80
	bx lr


.global svc_controlMemory
.type svc_controlMemory, %function
svc_controlMemory:
	stmfd sp!, {r0, r4}
	ldr r0, [sp, #0x8]
	ldr r4, [sp, #0x8+0x4]
	svc 0x01
	ldr r2, [sp], #4
	str r1, [r2]
	ldr r4, [sp], #4
	bx lr

.global svc_exitProcess
.type svc_exitProcess, %function
svc_exitProcess:
	svc 0x03
	bx lr

.global svc_createThread
.type svc_createThread, %function
svc_createThread:
	stmfd sp!, {r0, r4}
	ldr r0, [sp, #0x8]
	ldr r4, [sp, #0x8+0x4]
	svc 0x08
	ldr r2, [sp], #4
	str r1, [r2]
	ldr r4, [sp], #4
	bx lr

.global svc_exitThread
.type svc_exitThread, %function
svc_exitThread:
	svc 0x09
	bx lr

.global svc_sleepThread
.type svc_sleepThread, %function
svc_sleepThread:
	svc 0x0A
	bx lr

.global svc_createMutex
.type svc_createMutex, %function
svc_createMutex:
	str r0, [sp, #-4]!
	svc 0x13
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr

.global svc_releaseMutex
.type svc_releaseMutex, %function
svc_releaseMutex:
	svc 0x14
	bx lr

.global svc_releaseSemaphore
.type svc_releaseSemaphore, %function
svc_releaseSemaphore:
        str r0, [sp,#-4]!
        svc 0x16
        ldr r2, [sp], #4
        str r1, [r2]
        bx lr

.global svc_createEvent
.type svc_createEvent, %function
svc_createEvent:
	str r0, [sp,#-4]!
	svc 0x17
	ldr r2, [sp], #4
	str r1, [r2]
	bx lr

.global svc_signalEvent
.type svc_signalEvent, %function
svc_signalEvent:
	svc 0x18
	bx lr

.global svc_clearEvent
.type svc_clearEvent, %function
svc_clearEvent:
	svc 0x19
	bx lr

.global svc_createMemoryBlock
.type svc_createMemoryBlock, %function
svc_createMemoryBlock:
	str r0, [sp, #-4]!
	ldr r0, [sp, #4]
	svc 0x1E
	ldr r2, [sp], #4
	str r1, [r2]
	bx lr

.global svc_mapMemoryBlock
.type svc_mapMemoryBlock, %function
svc_mapMemoryBlock:
	svc 0x1F
	bx lr

.global svc_unmapMemoryBlock
.type svc_unmapMemoryBlock, %function
svc_unmapMemoryBlock:
	svc 0x20
	bx lr

.global svc_arbitrateAddress
.type svc_arbitrateAddress, %function
svc_arbitrateAddress:
        svc 0x22
        bx lr

.global svc_closeHandle
.type svc_closeHandle, %function
svc_closeHandle:
	svc 0x23
	bx lr

.global svc_waitSynchronization1
.type svc_waitSynchronization1, %function
svc_waitSynchronization1:
	svc 0x24
	bx lr

.global svc_waitSynchronizationN
.type svc_waitSynchronizationN, %function
svc_waitSynchronizationN:
	str r5, [sp, #-4]!
	mov r5, r0
	ldr r0, [sp, #0x4]
	ldr r4, [sp, #0x4+0x4]
	svc 0x25
	str r1, [r5]
	ldr r5, [sp], #4
	bx lr

.global svc_getSystemTick
.type svc_getSystemTick, %function
svc_getSystemTick:
	svc 0x28
	bx lr

.global svc_getSystemInfo
.type svc_getSystemInfo, %function
svc_getSystemInfo:
	stmfd sp!, {r0, r4}
	svc 0x2A
	ldr r4, [sp], #4
	str r1, [r4]
	str r2, [r4, #4]
	# str r3, [r4, #8] # ?
	ldr r4, [sp], #4
	bx lr

.global svc_getProcessInfo
.type svc_getProcessInfo, %function
svc_getProcessInfo:
	stmfd sp!, {r0, r4}
	svc 0x2B
	ldr r4, [sp], #4
	str r1, [r4]
	str r2, [r4, #4]
	ldr r4, [sp], #4
	bx lr

.global svc_connectToPort
.type svc_connectToPort, %function
svc_connectToPort:
	str r0, [sp,#-0x4]!
	svc 0x2D
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr

.global svc_sendSyncRequest
.type svc_sendSyncRequest, %function
svc_sendSyncRequest:
	svc 0x32
	bx lr

.global svc_getProcessId
.type svc_getProcessId, %function
svc_getProcessId:
	str r0, [sp,#-0x4]!
	svc 0x35
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr
	
.global svc_getThreadId
.type svc_getThreadId, %function
svc_getThreadId:
	str r0, [sp,#-0x4]!
	svc 0x37
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr 
	
	
.global svc_setThreadIdealProcessor
.type svc_setThreadIdealProcessor, %function
svc_setThreadIdealProcessor:
	svc 0x10
	bx lr
	
.global svc_openThread
.type svc_openThread, %function
svc_openThread:
	str r0, [sp,#-0x4]!
	svc 0x34
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr 

.global svc_flushProcessDataCache
.type svc_flushProcessDataCache, %function
svc_flushProcessDataCache:
	svc 0x54
	bx lr
	
.global svc_invalidateProcessDataCache
.type svc_invalidateProcessDataCache, %function
svc_invalidateProcessDataCache:
	svc 0x52
	bx lr
	
.global svc_queryMemory
.type svc_queryMemory, %function
svc_queryMemory:
	svc 0x02
	bx lr
	
.global svc_addCodeSegment
.type svc_addCodeSegment, %function
svc_addCodeSegment:
	svc 0x7a
	bx lr
	
.global svc_openProcess
.type svc_openProcess, %function
svc_openProcess:
	str r0, [sp,#-0x4]!
	svc 0x33
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr 

.global svc_controlProcessMemory
.type svc_controlProcessMemory, %function
svc_controlProcessMemory:

	stmfd sp!, {r0, r4, r5}
	ldr r4, [sp, #0xC+0x0]
	ldr r5, [sp, #0xC+0x4]
	svc 0x70
	ldmfd sp!, {r2, r4, r5}
	bx lr
	
	
.global svc_mapProcessMemory
.type svc_mapProcessMemory, %function
svc_mapProcessMemory:

	svc 0x71
	bx lr
	
.global svc_startInterProcessDma
.type svc_startInterProcessDma, %function
svc_startInterProcessDma:

	stmfd sp!, {r0, r4, r5}
	ldr r0, [sp, #0xC]
	ldr r4, [sp, #0xC+0x4]
	ldr r5, [sp, #0xC+0x8]
	svc 0x55
	ldmfd sp!, {r2, r4, r5}
	str	r1, [r2]
	bx lr

.global svc_getDmaState
.type svc_getDmaState, %function
svc_getDmaState:

	str r0, [sp,#-0x4]!
	svc 0x57
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr 

	
.global svc_backDoor
.type svc_backDoor, %function
svc_backDoor:
	svc 0x7b
	bx lr
	

.global svc_getProcessList
.type svc_getProcessList, %function
svc_getProcessList:
	str r0, [sp,#-0x4]!
	svc 0x65
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr 
	

.global svc_getThreadList
.type svc_getThreadList, %function
svc_getThreadList:
	str r0, [sp,#-0x4]!
	svc 0x66
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr 
	
.global svc_getThreadContext
.type svc_getThreadContext, %function
svc_getThreadContext:
	svc 0x3b
	bx lr 
	

	
.global svc_debugActiveProcess
.type svc_debugActiveProcess, %function
svc_debugActiveProcess:
	str r0, [sp,#-0x4]!
	svc 0x60
	ldr r3, [sp], #4
	str r1, [r3]
	bx lr 

.global svc_readProcessMemory	
.type svc_readProcessMemory, %function
svc_readProcessMemory:
	svc 0x6a
	bx lr
	
.global svc_writeProcessMemory	
.type svc_writeProcessMemory, %function
svc_writeProcessMemory:
	svc 0x6b
	bx lr
	
	
	
	
	
	