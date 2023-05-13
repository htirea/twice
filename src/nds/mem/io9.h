#ifndef TWICE_IO9_H
#define TWICE_IO9_H

#include "nds/math.h"
#include "nds/mem/io.h"

namespace twice {

void wramcnt_write(NDS *nds, u8 value);

inline u8
io9_read8(NDS *nds, u32 addr)
{
	switch (addr) {
	default:
		IO_READ8_COMMON(0);
	case 0x4000247:
		return nds->wramcnt;
	}
}

inline u16
io9_read16(NDS *nds, u32 addr)
{
	switch (addr) {
	default:
		IO_READ16_COMMON(0);
	case 0x40002B0:
		return nds->sqrtcnt;
	}
}

inline u32
io9_read32(NDS *nds, u32 addr)
{
	switch (addr) {
	default:
		IO_READ32_COMMON(0);
	case 0x40002B4:
		return nds->sqrt_result;
	case 0x40002B8:
		return nds->sqrt_param[0];
	case 0x40002BC:
		return nds->sqrt_param[1];
	}
}

inline void
io9_write8(NDS *nds, u32 addr, u8 value)
{
	switch (addr) {
	default:
		IO_WRITE8_COMMON(0);
	case 0x4000247:
		wramcnt_write(nds, value);
		break;
	}
}

inline void
io9_write16(NDS *nds, u32 addr, u16 value)
{
	switch (addr) {
	default:
		IO_WRITE16_COMMON(0);
	case 0x40002B0:
		/* TODO: sqrt timings */
		nds->sqrtcnt = (nds->sqrtcnt & 0x8000) | (value & ~0x8000);
		nds_math_sqrt(nds);
		break;
	}
}

inline void
io9_write32(NDS *nds, u32 addr, u32 value)
{
	switch (addr) {
	default:
		IO_WRITE32_COMMON(0);
	case 0x40002B8:
		nds->sqrt_param[0] = value;
		nds_math_sqrt(nds);
		break;
	case 0x40002BC:
		nds->sqrt_param[1] = value;
		nds_math_sqrt(nds);
		break;
	}
}

} // namespace twice

#endif
