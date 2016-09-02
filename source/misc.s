
.global backdoorHandler
.type backdoorHandler, %function	
backdoorHandler:
	ldr r3, [sp]
	push {lr}
	str sp, [r3, #-0x4]!
	mov sp, r3
	bl kernelCallback
	ldr r3, [sp], #4
	mov sp, r3
	pop {pc}

.global InvalidateEntireInstructionCache
.type InvalidateEntireInstructionCache, %function
InvalidateEntireInstructionCache:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	bx lr

.global InvalidateEntireDataCache
.type InvalidateEntireDataCache, %function
InvalidateEntireDataCache:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 0
	bx lr
	
.global FlushAllCache
.type FlushAllCache, %function
FlushAllCache:
mov r0, #0
mcr p15, 0, r0, c7, c14, 0 @Clean and Invalidate Entire Data Cache
mcr p15, 0, r0, c7, c5, 0 @Invalidate Entire Instruction Cache. Also flushes the branch target cache
mcr p15, 0, R0,c7,c10, 4 @Data Synchronization Barrier
mcr p15, 0, R0,c7,c5, 4 @Flush Prefetch Buffer
bx lr

_flushEntireDataCache:
	MOV R0, #0
	MCR p15, 0, R0, c7, c14, 0
	MCR p15, 0, R0, c7, c10, 4
	BX LR
.global kFlushDataCache
.type kFlushDataCache, %function
kFlushDataCache:
	CMP R1, #0x4000
	BCS _flushEntireDataCache
	BIC R2, R0, #0x1F
	ADD R0, R0, R1
	ADD R0, R0, #0x1F
	BIC R0, R0, #0x1F
	CMP R2, R0
	BCS _flushDataCache_end
1:
	MCR p15, 0, R2, c7, c14, 1
	ADD R2, R2, #0x20
	CMP R2, R0
	BCC 1b
_flushDataCache_end:
	MOV R0, #0
	MCR p15, 0, R0, c7, c10, 4
	BX LR