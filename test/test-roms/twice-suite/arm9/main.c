#include "font8x8_basic.h"
#include "util.h"

int test_writeback();

int
test_io()
{
	*((vu32 *)(0x4000304)) = 0xABCDF0F3;
	u32 num = *((vu32 *)(0x4000304));

	if (num != 0x8003) {
		return 1;
	}

	if (REG_POWCNT1 != 0x8003) {
		return 2;
	}

	return 0;
}

void
run_tests(void)
{
	int err = test_writeback();
	if (err) {
		print_string("writeback test failed", 8, 0, RED);
		print_hex(err, 18, 0, RED);
		return;
	}

	err = test_io();
	if (err) {
		print_string("io test failed", 8, 0, RED);
		print_hex(err, 18, 0, RED);
		return;
	}

	print_string("All tests passed!", 8, 0, GREEN);
}

void
arm9_main(void)
{
	REG_POWCNT1 = 0x8003;
	REG_DISPCNT_A = 0x20000;
	REG_VRAMCNT_A = 0x80;

	for (u32 i = 0; i < 0xC000; i++) {
		*((vu16 *)LCDC_OFFSET + i) = 0x0;
	}

	run_tests();

	for (;;) {
		;
	}
}

void
print_string(const char *s, int row, int col, u16 color)
{
	while (*s) {
		print_char(*s, row, col, color);

		col += 8;
		if (col >= FB_WIDTH) {
			col = 0;
			row += 8;
		}

		s++;
	}
}

void
print_char(char c, int row, int col, u16 color)
{
	char *letter = font8x8_basic[(unsigned char)c];

	for (int j = 0; j < 8; j++) {
		u8 bitmap = letter[j];
		for (int i = 0; i < 8; i++) {
			if (bitmap & (1 << i)) {
				*((vu16 *)LCDC_OFFSET + FB_WIDTH * (row + j) +
						(col + i)) = color;
			}
		}
	}
}

int
num_to_char(int c)
{
	if (c < 10) {
		return '0' + c;
	} else if (c < 16) {
		return 'A' + c - 10;
	} else {
		return '?';
	}
}

void
print_hex(u32 num, int row, int col, u16 color)
{
	char s[] = "0x00000000";

	for (int i = 0; i < 8; i++) {
		int c = num_to_char(num & 0xF);
		s[9 - i] = c;
		num >>= 4;
	}

	print_string(s, row, col, color);
}
