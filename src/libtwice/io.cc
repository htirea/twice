#include "libtwice/io.h"
#include "libtwice/nds.h"

namespace twice {

void
wramcnt_write(NDS *nds, u8 value)
{
	nds->wramcnt = value;

	switch (value & 3) {
	case 0:
		nds->shared_wram_p[0] = nds->shared_wram;
		nds->shared_wram_mask[0] = 32_KiB - 1;
		nds->shared_wram_p[1] = nds->arm7_wram;
		nds->shared_wram_mask[1] = ARM7_WRAM_MASK;
		break;
	case 1:
		nds->shared_wram_p[0] = nds->shared_wram + 16_KiB;
		nds->shared_wram_mask[0] = 16_KiB - 1;
		nds->shared_wram_p[1] = nds->shared_wram;
		nds->shared_wram_mask[1] = 16_KiB - 1;
		break;
	case 2:
		nds->shared_wram_p[0] = nds->shared_wram;
		nds->shared_wram_mask[0] = 16_KiB - 1;
		nds->shared_wram_p[1] = nds->shared_wram + 16_KiB;
		nds->shared_wram_mask[1] = 16_KiB - 1;
		break;
	case 3:
		nds->shared_wram_p[0] = nds->shared_wram_null;
		nds->shared_wram_mask[0] = 0;
		nds->shared_wram_p[1] = nds->shared_wram;
		nds->shared_wram_mask[1] = 32_KiB - 1;
	}
}

u32
io9_read32(NDS *nds, u32 addr)
{
	fprintf(stderr, "nds9 io read 32 at %08X\n", addr);
	return 0;
}

u16
io9_read16(NDS *nds, u32 addr)
{
	fprintf(stderr, "nds9 io read 16 at %08X\n", addr);
	return 0;
}

u8
io9_read8(NDS *nds, u32 addr)
{
	fprintf(stderr, "nds9 io read 8 at %08X\n", addr);
	return 0;
}

void
io9_write32(NDS *nds, u32 addr, u32 value)
{
	fprintf(stderr, "nds9 io write 32 to %08X\n", addr);
}

void
io9_write16(NDS *nds, u32 addr, u16 value)
{
	fprintf(stderr, "nds9 io write 16 to %08X\n", addr);
}

void
io9_write8(NDS *nds, u32 addr, u8 value)
{
	fprintf(stderr, "nds9 io write 16 to %08X\n", addr);
}

u32
io7_read32(NDS *nds, u32 addr)
{
	fprintf(stderr, "nds7 io read 32 at %08X\n", addr);
	return 0;
}

u16
io7_read16(NDS *nds, u32 addr)
{
	fprintf(stderr, "nds7 io read 16 at %08X\n", addr);
	return 0;
}

u8
io7_read8(NDS *nds, u32 addr)
{
	fprintf(stderr, "nds7 io read 8 at %08X\n", addr);
	return 0;
}

void
io7_write32(NDS *nds, u32 addr, u32 value)
{
	fprintf(stderr, "nds7 io write 32 to %08X\n", addr);
}

void
io7_write16(NDS *nds, u32 addr, u16 value)
{
	fprintf(stderr, "nds7 io write 16 to %08X\n", addr);
}

void
io7_write8(NDS *nds, u32 addr, u8 value)
{
	fprintf(stderr, "nds7 io write 16 to %08X\n", addr);
}

} // namespace twice
