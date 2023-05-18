@ vim:ft=armv5

.macro check_load
	orr r0, #1
	orr r0, #2
	orr r0, #4
	orr r0, #8
	cmp r0, #15
	bne fail
.endm

.text
.global test_writeback

test_writeback:
	push {r4-r11, lr}

	@ test 1: ldr pc writeback
	mov r11, #1
	mov r0, #0
	@ldr r1, [pc], #4
	.long 0xE49F1004
	check_load

	@ test 2: ldrb pc writeback
	mov r11, #2
	mov r0, #0
	@ldrb r1, [pc], #4
	.long 0xE4DF1004
	check_load

	@ test 3: ldrh pc writeback
	mov r11, #3
	mov r0, #0
	@ldrh r1, [pc], #4
	.long 0xE0DF10B4
	check_load

	@ test 4: ldrsh pc writeback
	mov r11, #4
	mov r0, #0
	@ldrsh r1, [pc], #4
	.long 0xE0DF10F4
	check_load

	@ test 5: ldrsb pc writeback
	mov r11, #5
	mov r0, #0
	@ldrsb r1, [pc], #4
	.long 0xE0DF10D4
	check_load

	@ test 6: ldrd pc writeback
	mov r11, #6
	mov r0, #0
	@ldrd r2, [pc], #4
	.long 0xE0CF20D4
	check_load

	@ test 7: str pc writeback
	mov r11, #7
	mov r0, #0
	ldr r1, =0xE3800004
	@str r1, [pc, #4]!
	.long 0xE5AF1004
	orr r0, #1
	orr r0, #2
	nop
	orr r0, #8
	cmp r0, #15
	bne fail

	@ test 8: strb pc writeback
	mov r11, #8
	mov r0, #0
	mov r1, #04
	@strb r1, [pc, #4]!
	.long 0xE5EF1004
	orr r0, #1
	orr r0, #2
	.long 0xE3800000
	orr r0, #8
	cmp r0, #15
	bne fail

	@ test 9: strh pc writeback
	mov r11, #9
	mov r0, #0
	mov r1, #4
	@ strh r1, [pc, #4]!
	.long 0xE1EF10B4
	orr r0, #1
	orr r0, #2
	.long 0xE3800000
	orr r0, #8
	cmp r0, #15
	bne fail

	@ test 0xA: strd pc writeback
	mov r11, #0xA
	mov r0, #0
	ldr r2, =0xE3800004
	ldr r3, =0xE3800008
	@ strd r2, [pc, #4]!
	.long 0xE1EF20F4
	orr r0, #1
	orr r0, #2
	nop
	nop
	cmp r0, #15
	bne fail

	@ test 0xB: ldm pc writeback
	mov r11, #0xB
	mov r0, #0
	@ ldmia pc!, {r1, r2}
	.long 0xE8BF0006
	check_load

	@ test 0xC: stm pc writeback
	mov r11, #0xC
	mov r0, #1
	ldr r1, =0xE3800004
	ldr r2, =0xE3800008
	@ stmib pc!, {r1, r2}
	.long 0xE9AF0006
	orr r0, #1
	orr r0, #2
	nop
	nop
	cmp r0, #15
	bne fail

success:
	mov r0, #0

return:
	pop {r4-r11, pc}
fail:
	mov r0, r11
	b return
