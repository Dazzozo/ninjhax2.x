.nds

.Create "cn_bootloader.bin",0x0

.include "../build/constants.s"

.orga 0x0
	.arm
		;r0 contains the HB handle
		;if r0 is 0x0 then load from mem
		cmp r0, #0
		beq notGotHb
			gotHb:
				ldr r10, =CN_HBHANDLE_LOC
				str r0, [r10]
				mov r10, r0
				mov r9, #0
				b doneHb
			notGotHb:
				ldr r10, =CN_HBHANDLE_LOC
				ldr r10, [r10]
				mov r9, #1
		doneHb:

		;r1 contains the file handle
		mov r11, r1

		;hb:FlushInvalidateCache
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r0, =0x00010042
			str r0, [r8], #4
			ldr r0, =0x00100000
			str r0, [r8], #4
			ldr r0, =0x00000000
			str r0, [r8], #4
			ldr r0, =0xFFFF8001
			str r0, [r8], #4

			mov r0, r10
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;induce crash if there's an error
			cmp r0, #0
			mrceq p15, 0, r8, c13, c0, 3
			ldreq r0, [r8, #0x84]
			cmpeq r0, #0
			ldrne r1, =0xBABE0081
			ldrne r1, [r1]

		;hb:Load3dsx
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r0, =0x00050042
			str r0, [r8], #4
			ldr r0, =0x00100000 ;baseAddr
			str r0, [r8], #4
			ldr r0, =0x00000000
			str r0, [r8], #4
			mov r0, r11 ;fileHandle
			str r0, [r8], #4

			mov r0, r10
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;induce crash if there's an error
			cmp r0, #0
			mrceq p15, 0, r8, c13, c0, 3
			ldreq r0, [r8, #0x84]
			cmpeq r0, #0
			ldrne r1, =0xBABE0083
			ldrne r1, [r1]

		;FSFILE:Close
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r0, =0x08080000
			str r0, [r8], #4

			mov r0, r11 ;fileHandle
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;induce crash if there's an error
			cmp r0, #0
			mrceq p15, 0, r8, c13, c0, 3
			ldreq r0, [r8, #0x84]
			cmpeq r0, #0
			ldrne r1, =0xBABE0088
			ldrne r1, [r1]

		;grab all handles from hb:
			bl initHandleTable

			mov r4, #0
			grabHandleLoop:
				mov r0, r4 ; handle index
				mov r1, r10 ; hb handle
				bl grabAndPushHandle
				add r4, #1
				cmp r4, HB_NUM_HANDLES
				blt grabHandleLoop

		mov sp, #0x10000000

		ldr r4, =0xDEADCAF3
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

		ldr lr, =CN_MENULOADER_LOC
		ldr pc, =0x00100000

	.pool

	; r0 : handle index
	; r1 : hb handle
	grabAndPushHandle:
		stmfd sp!, {r4-r5}
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r4, =0x00040040
			str r4, [r8]
			str r0, [r8, #0x4]

			mov r0, r1
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			; return if there's an error
			cmp r0, #0
			ldreq r0, [r8, #0x4]
			cmpeq r0, #0
			bne grabAndPushHandleEnd

			; store the handle
			ldr r0, =CN_SERVICESTRUCT_LOC
			ldr r5, [r0]

			add r3, r0, r5, lsl 2
			add r3, r3, r5, lsl 3

			ldr r4, [r8, #0x8]
			str r4, [r3, #0x4]
			ldr r4, [r8, #0xC]
			str r4, [r3, #0x8]
			ldr r4, [r8, #0x14]
			str r4, [r3, #0xC]
			
			add r5, #1
			str r5, [r0]

		grabAndPushHandleEnd:
		ldmfd sp!, {r4-r5}
		bx lr

	initHandleTable:
			ldr r0, =CN_SERVICESTRUCT_LOC
			mov r1, #0
			str r1, [r0]
		bx lr

	.pool

.orga CN_MENULOADER_LOC-CN_BOOTLOADER_LOC
	;grab hb handle
		ldr r10, =CN_HBHANDLE_LOC
		ldr r10, [r10]

	;set argc to 0
		ldr r0, =CN_ARGCV_LOC
		mov r1, #0
		str r1, [r0]

	;grab fs handle
		;hb:GetHandle(fs:USER)
			mrc p15, 0, r8, c13, c0, 3
			add r8, #0x80

			ldr r0, =0x00040040
			str r0, [r8]
			ldr r0, =0x00000000 ; index
			str r0, [r8, #0x4]

			mov r0, r10
			.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

			;r11 is fs:USER handle
			ldr r11, [r8, #0x14]

			;induce crash if there's an error
			cmp r0, #0
			ldreq r0, [r8, #0x4]
			cmpeq r0, #0
			ldrne r1, =0xBABE0084
			ldrne r1, [r1]

	;FSUSER:OpenFileDirectly
		mrc p15, 0, r8, c13, c0, 3
		add r8, #0x80

		ldr r0, =0x08030204
		str r0, [r8], #4
		ldr r0, =0x00000000 ; transaction
		str r0, [r8], #4
		ldr r0, [sdmcArchive] ; archive id
		str r0, [r8], #4
		ldr r0, [sdmcArchive+0x4] ; archive lowpath_type
		str r0, [r8], #4
		ldr r0, [sdmcArchive+0x8] ; archive lowpath_size
		str r0, [r8], #4
		ldr r0, [filePath] ; filepath type
		str r0, [r8], #4
		ldr r0, [filePath+0x4] ; filepath size
		str r0, [r8], #4
		ldr r0, =0x00000001 ; openflags
		str r0, [r8], #4
		ldr r0, =0x00000000 ; attributes
		str r0, [r8], #4
		ldr r0, [sdmcArchive+0x8] ; archive lowpath_size
		mov r0, r0, lsl 14
		ldr r1, =0x802
		orr r0, r1
		str r0, [r8], #4
		ldr r0, [sdmcArchive+0xC] ; archive lowpath_data
		str r0, [r8], #4
		ldr r0, [filePath+0x4] ; archive lowpath_size
		mov r0, r0, lsl 14
		orr r0, #2
		str r0, [r8], #4
		ldr r0, [filePath+0x8] ; filepath data
		str r0, [r8], #4

		mov r0, r11
		.word 0xEF000032 ; svc 0x32 (SendSyncRequest)

		;induce crash if there's an error
		cmp r0, #0
		mrceq p15, 0, r8, c13, c0, 3
		ldreq r0, [r8, #0x84]
		cmpeq r0, #0
		ldrne r1, =0xBAD00001
		ldrne r1, [r1]

	mov r0, #0
	ldr r1, [r8, #0x8C]
	ldr pc, =CN_BOOTLOADER_LOC

	sdmcArchive:
		.word 0x00000009 ; archive ID
		.word 0x00000001 ; lowpath type (PATH_EMTPY)
		.word 0x00000001 ; lowpath length
		.word CN_BOOTLOADER_LOC+filePathData ; lowpath data ptr
	filePath:
		.word 0x00000003 ; lowpath type (PATH_CHAR)
		.word 0x0000000B ; lowpath length
		.word CN_BOOTLOADER_LOC+filePathData ; lowpath data ptr
	filePathData:
		.ascii "/boot.3dsx"
		.byte 0x00
	.pool


.orga CN_ARGSETTER_LOC-CN_BOOTLOADER_LOC
	;simply copy r1 bytes from r0 to CN_ARGCV_LOC
	;might copy 3 bytes too many but tbh idgaf
	;also no alignment no shoes no service
	;also set CN_ARGCV_LOC[0] to 0 if r1=0
	ldr r2, =CN_ARGCV_LOC
	cmp r1, #0
	streq r1, [r2]
	beq argcpyret
	add r1, r0
	argcpyloop:
		ldr r3, [r0], #4
		str r3, [r2], #4
		cmp r0, r1
		blt argcpyloop
	argcpyret:
	bx lr

.close
