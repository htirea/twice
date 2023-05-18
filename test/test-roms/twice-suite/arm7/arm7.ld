ENTRY(_arm7_start)

SECTIONS
{
	. = 0x3800000;
	.text :
	{
		entry.o (.text)
	}
}
