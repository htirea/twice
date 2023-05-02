#ifndef LIBTWICE_IO_H
#define LIBTWICE_IO_H

#include "libtwice/types.h"

namespace twice {

struct NDS;

u32 io9_read32(NDS *nds, u32 addr);
u16 io9_read16(NDS *nds, u32 addr);
u8 io9_read8(NDS *nds, u32 addr);
void io9_write32(NDS *nds, u32 addr, u32 value);
void io9_write16(NDS *nds, u32 addr, u16 value);
void io9_write8(NDS *nds, u32 addr, u8 value);

u32 io7_read32(NDS *nds, u32 addr);
u16 io7_read16(NDS *nds, u32 addr);
u8 io7_read8(NDS *nds, u32 addr);
void io7_write32(NDS *nds, u32 addr, u32 value);
void io7_write16(NDS *nds, u32 addr, u16 value);
void io7_write8(NDS *nds, u32 addr, u8 value);

void wramcnt_write(NDS *nds, u8 value);

template <typename T, int cpu>
T
bus_io_read(NDS *nds, u32 addr)
{
	if constexpr (cpu == 0 && sizeof(T) == 1) {
		return io9_read8(nds, addr);
	} else if constexpr (cpu == 0 && sizeof(T) == 2) {
		return io9_read16(nds, addr);
	} else if constexpr (cpu == 0 && sizeof(T) == 4) {
		return io9_read32(nds, addr);
	} else if constexpr (cpu == 1 && sizeof(T) == 1) {
		return io7_read8(nds, addr);
	} else if constexpr (cpu == 1 && sizeof(T) == 2) {
		return io7_read16(nds, addr);
	} else if constexpr (cpu == 1 && sizeof(T) == 4) {
		return io7_read32(nds, addr);
	}
}

template <typename T, int cpu>
void
bus_io_write(NDS *nds, u32 addr, T value)
{
	if constexpr (cpu == 0 && sizeof(T) == 1) {
		io9_write8(nds, addr, value);
	} else if constexpr (cpu == 0 && sizeof(T) == 2) {
		io9_write16(nds, addr, value);
	} else if constexpr (cpu == 0 && sizeof(T) == 4) {
		io9_write32(nds, addr, value);
	} else if constexpr (cpu == 1 && sizeof(T) == 1) {
		io7_write8(nds, addr, value);
	} else if constexpr (cpu == 1 && sizeof(T) == 2) {
		io7_write16(nds, addr, value);
	} else if constexpr (cpu == 1 && sizeof(T) == 4) {
		io7_write32(nds, addr, value);
	}
}

} // namespace twice

#endif
