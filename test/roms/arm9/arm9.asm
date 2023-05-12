@ vim:ft=armv5

.text
.global _arm9_start

_arm9_start:
.include "../common_setup.asm"

@ Test 1: ldr: writeback with pc as base register
t1:
        mov r11, #0x1

        mov r4, #0x1
        @ ldr r0, [pc, #4]!
        .int 0xE5BF0004
	orr r4, #0x2
	orr r4, #0x4
	orr r4, #0x8
	orr r4, #0x10
	orr r4, #0x20
	orr r4, #0x40
	orr r4, #0x80

	cmp r4, #0xFF
        bne fail_test

@ Test 2: str: writeback with pc as base register
t2:
	mov r11, #0x2

	mov r4, #0x1
	ldr r0, =0xE3844008
	@ str r0, [pc, #4]!
	.int 0xE5AF0004
	orr r4, #0x2
	orr r4, #0x4
	nop
	orr r4, #0x10
	orr r4, #0x20
	orr r4, #0x40
	orr r4, #0x80

	cmp r4, #0xFF
	bne fail_test

.include "../common_main.asm"

.end
