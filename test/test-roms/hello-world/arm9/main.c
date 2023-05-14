#include "font8x8_basic.h"
#include "util.h"

void
arm9_main(void)
{
	REG_POWCNT1 = 0x8003;
	REG_DISPCNT_A = 0x20000;
	REG_VRAMCNT_A = 0x80;

	for (u32 i = 0; i < 0xC000; i++) {
		*((vu16 *)LCDC_OFFSET + i) = 0x0;
	}

	print_string("Hello World!", 0, 0, WHITE);

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
