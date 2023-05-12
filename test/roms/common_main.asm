	@ Print success message
	ldr r0, =success_message
	mov r1, #0
	mov r2, #0
	bl print_string

	ldr r0, =num_tests
	mov r1, #8
	mov r2, #0
	bl print_string

	mov r0, r11
	mov r1, #8
	mov r2, #160
	bl print_hex

forever:
	b forever

@ test_num in r11
fail_test:
        mov r0, r11
        mov r1, #8
        mov r2, #0
        bl print_hex
        b forever

@ str, row, col
print_string:
	push {r4, r5, r6, lr}
	mov r4, r0
	mov r5, r1
	mov r6, r2

0:
	ldrb r0, [r4], #1
	cmp r0, #0
	beq print_string_end
	mov r1, r5
	mov r2, r6
	bl print_char

	add r6, #8
	cmp r6, #256
	movhi r6, #0
	addhi r5, #12
	b 0b

print_string_end:
	pop {r4, r5, r6, pc}

@ num, row, col
print_hex:
	push {r4, r5, r6, r7, lr}
	mov r4, r0
	mov r5, r1
	mov r6, r2

	mov r0, #'0'
	mov r1, r5
	mov r2, r6
	bl print_char

	add r6, #8
	cmp r6, #256
	movhi r6, #0
	addhi r5, #12

	mov r0, #'x'
	mov r1, r5
	mov r2, r6
	bl print_char

	add r6, #8
	cmp r6, #256
	movhi r6, #0
	addhi r5, #12

	mov r7, #28
0:
	mov r0, r4, LSR r7
	and r0, #0xF
	mov r1, r5
	mov r2, r6
	bl print_digit

	add r6, #8
	cmp r6, #256
	movhi r6, #0
	addhi r5, #12

	subs r7, #4
	bhs 0b

	pop {r4, r5, r6, r7, pc}

@ digit, row, col
print_digit:
	mov r3, r0
	subs r12, r3, #0xA

	addlo r0, r3, #'0'
	blo print_char

	add r0, r12, #'A'
	b print_char

@ char, row, col
print_char:
	ldr r3, =font
	add r3, r3, r0, LSL #3
	mov r0, #0x6800000
	add r0, r1, LSL #9
	add r0, r2, LSL #1

	mov r1, #0xFF
	orr r1, #0x7F00

	mov r12, #0
0:
	ldrb r2, [r3], #1
	tst r2, #0x1
	strneh r1, [r0]
	tst r2, #0x2
	strneh r1, [r0, #2]
	tst r2, #0x4
	strneh r1, [r0, #4]
	tst r2, #0x8
	strneh r1, [r0, #6]
	tst r2, #0x10
	strneh r1, [r0, #8]
	tst r2, #0x20
	strneh r1, [r0, #10]
	tst r2, #0x40
	strneh r1, [r0, #12]
	tst r2, #0x80
	strneh r1, [r0, #14]

	add r0, #512
	add r12, #2
	cmp r12, #16
	bne 0b

	bx lr

@ The bitmap font to use: should be 128 * 8 = 1024 bytes
@ You can use any bitmap font
font:
.incbin "../font8x8.bin"

success_message: .asciz "All tests passed!"
num_tests: .asciz "Number of tests: "
