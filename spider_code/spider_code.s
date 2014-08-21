.nds

.include "../build/constants.s"

.Create "spider_code.bin",0x0

;spider code
.arm
	ldr r2, =0x00100000+SPIDER_TEXT_LENGTH
	ldr r0, =0xEF000009 ; svc 0x09 (ExitThread)
	ldr r1, =0x00100000
	exitThreadLoop:
		str r0, [r1], #4
		cmp r1, r2
		blt exitThreadLoop

	;wake thread1
	ldr r1, =SPIDER_PROCSEMAPHORE_ADR
	ldr r1, [r1]
	mov r2, #1
	.word 0xEF000016 ; svc 0x16 (ReleaseSemaphore)

	;wake thread2
	ldr r0, =SPIDER_APTHANDLES_ADR+8
	ldr r0, [r0]
	.word 0xEF000018 ; svc 0x18 (SignalEvent)

	;wake thread3 and thread4
	ldr r0, =SPIDER_ADDRESSARBITER_ADR
	ldr r0, [r0] ; handle
	ldr r1, =SPIDER_ARBADDRESS_1 ;addr
	ldr r2, =0x00000000 ; arbitration type
	ldr r3, =0xFFFFFFFF ; value (-1)
	ldr r4, =0x00000000 ; nanoseconds
	ldr r5, =0x00000000 ; nanoseconds
	.word 0xEF000022 ; svc 0x22 (ArbitrateAddress)

	;wake thread7
	ldr r0, =SPIDER_ADDRESSARBITER_ADR
	ldr r0, [r0] ; handle
	ldr r1, =SPIDER_ARBADDRESS_2 ;addr
	ldr r2, =0x00000000 ; arbitration type
	ldr r3, =0xFFFFFFFF ; value (-1)
	ldr r4, =0x00000000 ; nanoseconds
	ldr r5, =0x00000000 ; nanoseconds
	.word 0xEF000022 ; svc 0x22 (ArbitrateAddress)

	;wake thread8
	ldr r0, =SPIDER_ADDRESSARBITER_ADR
	ldr r0, [r0] ; handle
	ldr r1, =SPIDER_ARBADDRESS_3 ;addr
	ldr r2, =0x00000000 ; arbitration type
	ldr r3, =0xFFFFFFFF ; value (-1)
	ldr r4, =0x00000000 ; nanoseconds
	ldr r5, =0x00000000 ; nanoseconds
	.word 0xEF000022 ; svc 0x22 (ArbitrateAddress)

	;wake thread10
	ldr r0, =SPIDER_ADDRESSARBITER_ADR
	ldr r0, [r0] ; handle
	ldr r1, =SPIDER_ARBADDRESS_4 ;addr
	ldr r2, =0x00000000 ; arbitration type
	ldr r3, =0xFFFFFFFF ; value (-1)
	ldr r4, =0x00000000 ; nanoseconds
	ldr r5, =0x00000000 ; nanoseconds
	.word 0xEF000022 ; svc 0x22 (ArbitrateAddress)

	;sleep for a second
	ldr r0, =0x3B9ACA00
	ldr r1, =0x00000000
	.word 0xEF00000A ; sleep

	; ;unmap memory blocks
	; 	;addr 0x10000000
	; 		ldr r0, =SPIDER_HIDMEMHANDLE_ADR
	; 		ldr r0, [r0] ; handle
	; 		ldr r1, =0x10000000 ; addr

	; 		.word 0xEF000020 ; svc 0x20 (UnmapMemoryBlock)

	; 		;induce crash if there's an error
	; 		cmp r0, #0
	; 		ldrne r1, =0xCAFE0062
	; 		ldrne r1, [r1]

	; 	;addr 0x10002000
	; 		ldr r0, =SPIDER_GSPMEMHANDLE_ADR
	; 		ldr r0, [r0] ; handle
	; 		ldr r1, =0x10002000 ; addr

	; 		.word 0xEF000020 ; svc 0x20 (UnmapMemoryBlock)

	; 		;induce crash if there's an error
	; 		cmp r0, #0
	; 		ldrne r1, =0xCAFE0063
	; 		ldrne r1, [r1]

	; ;bruteforce-close all handles
	; 	;scanning data and .bss sections for handles (and closing them)
	; 	ldr r8, =0x003D1000
	; 	ldr r9, =0x003D1000+0x00017E80+0x00056830
	; 	ldr r10, =0x7FFF
	; 	ldr r11, =SPIDER_GSPHANDLE_ADR
	; 	ldr r11, [r11]
	; 	ldr r12, =SPIDER_ROHANDLE_ADR
	; 	ldr r12, [r12]
	; 	closeHandleLoop1:
	; 		ldr r0, [r8], #4
	; 		cmp r0, r11
	; 		beq endCloseHandleLoop1
	; 		cmp r0, r12
	; 		beq endCloseHandleLoop1
	; 		and r1, r0, r10
	; 		cmp r1, #0x30
	; 		bgt endCloseHandleLoop1
	; 		.word 0xEF000023 ; svc 0x23 (CloseHandle)
	; 		endCloseHandleLoop1:
	; 		cmp r8, r9
	; 		blt closeHandleLoop1

	;hand-closing handles stored on the heap

		;stray event
		ldr r0, =0x09a6c000+0x1BC
		ldr r0, [r0]
		.word 0xEF000023 ; svc 0x23 (CloseHandle)

		;stray mutex
		ldr r0, =0x080493f4
		ldr r0, [r0]
		.word 0xEF000023 ; svc 0x23 (CloseHandle)

		;stray timer
		ldr r0, =0x0804a9e0
		ldr r0, [r0]
		.word 0xEF000023 ; svc 0x23 (CloseHandle)

	;free GSP heap
		ldr r0, =0x00000001 ; type (FREE)
		ldr r1, =SPIDER_GSPHEAPSTART ; addr0
		ldr r2, =0x00000000 ; addr1
		ldr r3, =SPIDER_GSPHEAPSIZE ; size
		ldr r4, =0x00000000 ; permissions (RW)

		.word 0xEF000001 ; svc 0x01 (ControlMemory)

		;induce crash if there's an error
		cmp r0, #0
		ldrne r1, =0xCAFE0061
		ldrne r1, [r1]

	;reconnect to ro
		sub sp, #0x20
		;connect back to srv:
			ldr r1, =SPIDER_CROMAPADR+srvString+CRO_SPIDERCODE_OFFSET
			.word 0xEF00002D ; svc 0x2D (ConnectToPort)
			str r1, [sp]

			;induce crash if there's an error
			cmp r0, #0
			ldrne r1, =0xCAFE00080
			ldrne r1, [r1]
			
		;srv:Initialize
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80
			ldr r0, =0x00010002
			str r0, [r8], #4
			ldr r0, =0x00000020
			str r0, [r8], #4
			ldr r0, [sp]
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;induce crash if there's an error
			cmp r0, #0
			ldrne r1, =0xCAFE0081
			ldrne r1, [r1]

		;srv:GetServiceHandle("fs:USER")
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80
			ldr r0, =0x00050100
			str r0, [r8], #4
			ldr r0, =0x553A7366  ;fs:U
			str r0, [r8], #4
			ldr r0, =0x00524553  ;SER
			str r0, [r8], #4
			ldr r0, =0x00000007 ;strlen
			str r0, [r8], #4
			ldr r0, =0x00000000 ;0x0
			str r0, [r8], #4

			ldr r0, [sp]
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)
			ldr r1, [r8, #-0x8]
			str r1, [sp, #0xC]

			;induce crash if there's an error
			cmp r0, #0
			ldrne r1, =0xCAFE0082
			ldrne r1, [r1]

			ldr r1, =SPIDER_ROHANDLE_ADR
			ldr r1, [r1]
			str r1, [sp, #4]

		;FS:Initialize
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r0, =0x08010002
			str r0, [r8], #4
			ldr r0, =0x00000020
			str r0, [r8], #4

			ldr r0, [sp, 0xC] ; fs:USER handle
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;induce crash if there's an error
			cmp r0, #0
			ldrne r1, =0xCAFE007F
			ldrne r1, [r1]

		;hb:SendHandle
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r0, =0x00030042
			str r0, [r8], #4
			ldr r0, =0x00000000
			str r0, [r8], #4
			ldr r0, =0x00000000
			str r0, [r8], #4
			ldr r0, [sp, 0xC] ; fs:USER handle
			str r0, [r8], #4

			ldr r0, [sp, #4]
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;induce crash if there's an error
			cmp r0, #0
			ldrne r1, =0xCAFE0083
			ldrne r1, [r1]


		;srv:GetServiceHandle("APT:U")
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80
			ldr r0, =0x00050100
			str r0, [r8], #4
			ldr r0, =0x3A545041  ;APT:
			str r0, [r8], #4
			ldr r0, =0x00000055  ;U
			str r0, [r8], #4
			ldr r0, =0x00000005 ;strlen
			str r0, [r8], #4
			ldr r0, =0x00000000 ;0x0
			str r0, [r8], #4

			ldr r0, [sp]
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)
			ldr r1, [r8, #-0x8]
			str r1, [sp, #8]

			;induce crash if there's an error
			cmp r0, #0
			ldrne r1, =0xCAFE0082
			ldrne r1, [r1]

		;APT:JumpToApplication
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r0, =0x00240044 ;cmd header
			str r0, [r8], #4
			ldr r0, =0x00000000 ;arg size
			str r0, [r8], #4
			ldr r0, =0x00000000 ;val 0x0
			str r0, [r8], #4
			ldr r0, [sp, #4] ;arg handle (ldr:ro)
			str r0, [r8], #4
			ldr r0, =0x00000002 ;(arg size << 14)|2
			str r0, [r8], #4
			ldr r0, =SPIDER_CROMAPADR ;arg buffer addr
			str r0, [r8], #4

			ldr r0, [sp, #8]
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;induce crash if there's an error
			cmp r0, #0
			ldrne r1, =0xCAFE0095
			ldrne r1, [r1]

			;induce crash if there's an error
			mrc p15, 0, r8, c13, c0, 3
			ldr r8, [r8,#0x84]
			cmp r8, #0
			ldrne r1, =0xCAFE0096
			ldrne r1, [r1]

		;close handle (APT:U)
			ldr r0, [sp, #8]
			.word 0xEF000023 ; svc 0x23 (CloseHandle)


		;close handle (fs:USER)
			ldr r0, [sp, #0xC]
			.word 0xEF000023 ; svc 0x23 (CloseHandle)

		;close handle (ldr:ro)
			ldr r0, [sp, #4]
			.word 0xEF000023 ; svc 0x23 (CloseHandle)

		;close handle (srv:)
			ldr r0, [sp]
			.word 0xEF000023 ; svc 0x23 (CloseHandle)
		add sp, #0x20


	;GSPGPU_ReleaseRight
		mrc p15, 0, r8, c13, c0, 3
		add r8, #0x80
		ldr r0, =0x00170000
		str r0, [r8], #4
		ldr r0, =SPIDER_GSPHANDLE_ADR
		ldr r0, [r0]
		.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

		;induce crash if there's an error
		cmp r0, #0
		ldrne r1, =0xCAFE0021
		ldrne r1, [r1]

	; mov sp, #0x10000000

	ldr r4, =0xDEADCAF2
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!
	str r4, [sp, #-4]!

	; .word 0xEF000003 ; svc 0x03 (ExitProcess)

			inftest:
				;sleep for a second
				ldr r0, =0x3B9ACA00
				ldr r1, =0x00000000
				.word 0xEF00000A ; sleep
				b inftest

	; ldr pc, =0x00100000

	; ldr r4, =0xDEADCAFE
	; ldr r4, [r4]

	; inf2:
	; 	b inf2

	.pool

srvString:
	.ascii "srv:"
	.byte 0x00

.close
