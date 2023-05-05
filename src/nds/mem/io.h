#ifndef TWICE_IO_H
#define TWICE_IO_H

#include "nds/nds.h"

namespace twice {

struct NDS;

void wramcnt_write(NDS *nds, u8 value);

template <typename T>
T
io9_read(NDS *nds, u32 addr)
{
	(void)nds;

	fprintf(stderr, "nds9 io read %lu at %08X\n", sizeof(T), addr);
	return 0;
}

template <typename T>
void
io9_write(NDS *nds, u32 addr, T value)
{
	(void)nds;
	(void)value;

	fprintf(stderr, "nds9 io write %lu to %08X\n", sizeof(T), addr);
}

template <typename T>
T
io7_read(NDS *nds, u32 addr)
{
	(void)nds;

	fprintf(stderr, "nds9 io read %lu at %08X\n", sizeof(T), addr);
	return 0;
}

template <typename T>
void
io7_write(NDS *nds, u32 addr, T value)
{
	(void)nds;
	(void)value;

	fprintf(stderr, "nds9 io write %lu to %08X\n", sizeof(T), addr);
}

} // namespace twice

#endif
