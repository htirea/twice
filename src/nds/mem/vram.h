#ifndef TWICE_VRAM_H
#define TWICE_VRAM_H

#include "nds/nds.h"

#include "common/util.h"

namespace twice {

template <typename T>
T
vram_read(NDS *nds, u32 addr)
{
	(void)nds;

	fprintf(stderr, "vram read at %08X\n", addr);
	return 0;
}

template <typename T>
void
vram_write(NDS *nds, u32 addr, T value)
{
	writearr<T>(nds->vram, addr - 0x6800000, value);
}

template <typename T>
T
vram_arm7_read(NDS *nds, u32 addr)
{
	(void)nds;

	fprintf(stderr, "vram arm7 read at %08X\n", addr);
	return 0;
}

template <typename T>
void
vram_arm7_write(NDS *nds, u32 addr, T value)
{
	(void)nds;
	(void)value;
	fprintf(stderr, "vram arm7 write to %08X\n", addr);
}

} // namespace twice

#endif
