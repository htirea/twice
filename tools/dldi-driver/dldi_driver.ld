ENTRY(_start)

SECTIONS
{
	. = 0x00000000;
	.text :
	{
		header.o (.text)
	}
}
