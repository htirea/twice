	@ IO offset
	mov r0, #0x4000000

	@ POWCNT1: turn screens on, enable engine A, send A to top screen
	mov r1, #0x8000
	add r1, #0x3
	str r1, [r0, #0x304]

	@ DISPCNT: set vram display mode
	mov r1, #0x20000
	str r1, [r0]

	@ VRAMCNT_A: enable bank A and set to LCDC
	mov r1, #0x80
	strb r1, [r0, #0x240]

	@ LCDC offset
	mov r0, #0x6800000

	@ Paint whole screen black
	mov r1, #0x0
	mov r2, #0x6000

0:
	str r1, [r0], #4
	subs r2, #1
	bne 0b
