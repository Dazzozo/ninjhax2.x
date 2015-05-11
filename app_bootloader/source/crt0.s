.section ".init"
.arm
.align 4
.global _start

_start:
	@ allocate bss/heap
	@ no need to initialize as OS does that already.
	@ need to save registers because _main takes parameters
	stmfd sp!, {r0, r1, r2, r3, r4}

		@ LINEAR MEMOP_COMMIT
		ldr r0, =0x10003
		@ addr0
		mov r1, #0
		@ addr1
		mov r2, #0
		@ size
		ldr r3, =_heap_size
		ldr r3, [r3]
		@ RW permissions
		mov r4, #3

		@ svcControlMemory
		svc 0x01

		@ save linear heap 
		ldr r2, =_heap_base
		str r1, [r2]

	ldmfd sp!, {r0, r1, r2, r3, r4}

	blx _main
