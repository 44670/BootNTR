
.global backdoorHandler
.type backdoorHandler, %function	
backdoorHandler:
cpsid	aif
STMFD   SP!, {R3-R11,LR}
bl		kernelCallback
LDMFD   SP!, {R3-R11,PC}

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
