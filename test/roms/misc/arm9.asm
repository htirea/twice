@ vim:ft=armv5

.text
.global _arm9_start

_arm9_start:
.include "../common_setup.asm"

	@ IME test
	mov r11, #0x1

	ldr r4, =#0x4000208

	ldr r5, =#0x12345678
	str r5, [r4]

	ldr r0, [r4]
	cmp r0, #0
	bne fail_test

	ldrb r0, [r4]
	cmp r0, #0
	bne fail_test

	ldrh r0, [r4]
	cmp r0, #0
	bne fail_test

	ldr r5, =#0x12345679
	str r5, [r4]

	ldr r0, [r4]
	cmp r0, #1
	bne fail_test

	ldrb r0, [r4]
	cmp r0, #1
	bne fail_test

	ldrh r0, [r4]
	cmp r0, #1
	bne fail_test


.include "../common_main.asm"

.end
