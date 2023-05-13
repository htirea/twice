#ifndef TWICE_IO9_H
#define TWICE_IO9_H

#include "nds/mem/io.h"

namespace twice {

void wramcnt_write(NDS *nds, u8 value);

inline u8
io9_read8(NDS *nds, u32 addr)
{
	switch (addr) {
	default:
		IO_READ8_COMMON(0);
	}
}

inline u16
io9_read16(NDS *nds, u32 addr)
{
	switch (addr) {
	default:
		IO_READ16_COMMON(0);
	}
}

inline u32
io9_read32(NDS *nds, u32 addr)
{
	switch (addr) {
	default:
		IO_READ32_COMMON(0);
	}
}

inline void
io9_write8(NDS *nds, u32 addr, u8 value)
{
	switch (addr) {
	default:
		IO_WRITE8_COMMON(0);
	}
}

inline void
io9_write16(NDS *nds, u32 addr, u16 value)
{
	switch (addr) {
	default:
		IO_WRITE16_COMMON(0);
	}
}

inline void
io9_write32(NDS *nds, u32 addr, u32 value)
{
	switch (addr) {
	default:
		IO_WRITE32_COMMON(0);
	}
}

} // namespace twice

#endif
