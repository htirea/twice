#define REG_DLDI (*(volatile unsigned *)0x4EF0EF0)

int
startup()
{
	REG_DLDI = 1;
	return REG_DLDI == 0;
}

int
is_inserted()
{
	REG_DLDI = 2;
	return REG_DLDI == 0;
}

int
read_sectors(unsigned sector, unsigned num_sectors, char *buf)
{
	REG_DLDI = 3;
	REG_DLDI = sector;
	REG_DLDI = num_sectors;

	for (unsigned i = num_sectors * 512; i--;) {
		*buf++ = REG_DLDI;
	}

	return REG_DLDI == 0;
}

int
write_sectors(unsigned sector, unsigned num_sectors, const char *buf)
{
	REG_DLDI = 4;
	REG_DLDI = sector;
	REG_DLDI = num_sectors;

	for (unsigned i = num_sectors * 512; i--;) {
		REG_DLDI = *buf++;
	}

	return REG_DLDI == 0;
}

int
clear_status()
{
	REG_DLDI = 5;
	return REG_DLDI == 0;
}

int
shutdown()
{
	REG_DLDI = 6;
	return REG_DLDI == 0;
}
