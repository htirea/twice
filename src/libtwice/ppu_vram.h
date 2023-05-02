#ifndef LIBTWICE_PPU_VRAM_H
#define LIBTWICE_PPU_VRAM_H

#include "libtwice/nds.h"

namespace twice {

template <typename T>
T
vram_read(NDS *nds, u32 addr)
{
	fprintf(stderr, "vram read at %08X\n", addr);
	return 0;
}

template <typename T>
void
vram_write(NDS *nds, u32 addr, T value)
{
	fprintf(stderr, "vram write to %08X\n", addr);
}

template <typename T>
T
vram_arm7_read(NDS *nds, u32 addr)
{
	fprintf(stderr, "vram arm7 read at %08X\n", addr);
	return 0;
}

template <typename T>
void
vram_arm7_write(NDS *nds, u32 addr, T value)
{
	fprintf(stderr, "vram arm7 write to %08X\n", addr);
}

} // namespace twice

#endif
