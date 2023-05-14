ENTRY(_arm9_start)

SECTIONS
{
	. = 0x2000000;
	.text : { *(.text) *(.data) *(.bss) }
}
