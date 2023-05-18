@ vim:ft=armv5

.text
.global test_writeback

test_writeback:
	push {r4-r11, lr}

	@ test 1: ldr writeback
	mov r11, #1

	mov r0, #0
	@ldr r1, [pc], #4
	.long 0xE49F1004
	orr r0, #1
	orr r0, #2
	orr r0, #4
	orr r0, #8

	cmp r0, #15
	bne fail

success:
	mov r0, #0

return:
	pop {r4-r11, pc}
fail:
	mov r0, r11
	b return
