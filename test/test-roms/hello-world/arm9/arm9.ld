ENTRY(_arm9_start)

SECTIONS
{
	. = 0x2000000;
	.text :
	{
		entry.o (.text)
		*(.text)
		*(.data)
		*(.bss)
	}
}
