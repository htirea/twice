#ifndef TWICE_IO_H
#define TWICE_IO_H

#include "nds/nds.h"

namespace twice {

struct NDS;

void wramcnt_write(NDS *nds, u8 value);

inline u16
io9_read16(NDS *nds, u32 addr)
{
	switch (addr) {
	case 0x4000004:
		return nds->dispstat[0];
	case 0x4000130:
		return nds->keyinput;
	}

	fprintf(stderr, "nds9 io read 16 at %08X\n", addr);
	return 0;
}

inline u16
io7_read16(NDS *nds, u32 addr)
{
	switch (addr) {
	case 0x4000004:
		return nds->dispstat[1];
	case 0x4000130:
		return nds->keyinput;
	}

	fprintf(stderr, "nds7 io read 16 at %08X\n", addr);
	return 0;
}

template <typename T>
T
io9_read(NDS *nds, u32 addr)
{
	if constexpr (sizeof(T) == 2) {
		return io9_read16(nds, addr);
	}

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
	if constexpr (sizeof(T) == 2) {
		return io7_read16(nds, addr);
	}

	fprintf(stderr, "nds7 io read %lu at %08X\n", sizeof(T), addr);
	return 0;
}

template <typename T>
void
io7_write(NDS *nds, u32 addr, T value)
{
	(void)nds;
	(void)value;

	fprintf(stderr, "nds7 io write %lu to %08X\n", sizeof(T), addr);
}

} // namespace twice

#endif
