#ifndef TWICE_VRAM_H
#define TWICE_VRAM_H

#include "nds/nds.h"

#include "common/util.h"

#define VRAM_READ_FROM_MASK(bank_)                                            \
	if (mask & BIT(bank_))                                                \
	value |= nds->gpu.vram_bank_read<T, bank_>(offset)

#define VRAM_WRITE_FROM_MASK(bank_)                                           \
	if (mask & BIT(bank_))                                                \
	nds->gpu.vram_bank_write<T, bank_>(offset, value)

namespace twice {

template <typename T>
T
vram_read_abg(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 31;
	u8 *p = nds->gpu.abg_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->gpu.abg_bank[index];
		T value = 0;

		VRAM_READ_FROM_MASK(VRAM_A);
		VRAM_READ_FROM_MASK(VRAM_B);
		VRAM_READ_FROM_MASK(VRAM_C);
		VRAM_READ_FROM_MASK(VRAM_D);
		VRAM_READ_FROM_MASK(VRAM_E);
		VRAM_READ_FROM_MASK(VRAM_F);
		VRAM_READ_FROM_MASK(VRAM_G);

		return value;
	}
}

template <typename T>
void
vram_write_abg(NDS *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 31;
	u8 *p = nds->gpu.abg_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->gpu.abg_bank[index];
		VRAM_WRITE_FROM_MASK(VRAM_A);
		VRAM_WRITE_FROM_MASK(VRAM_B);
		VRAM_WRITE_FROM_MASK(VRAM_C);
		VRAM_WRITE_FROM_MASK(VRAM_D);
		VRAM_WRITE_FROM_MASK(VRAM_E);
		VRAM_WRITE_FROM_MASK(VRAM_F);
		VRAM_WRITE_FROM_MASK(VRAM_G);
	}
}

template <typename T>
T
vram_read_bbg(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->gpu.bbg_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->gpu.bbg_bank[index >> 1];
		T value = 0;

		VRAM_READ_FROM_MASK(VRAM_C);
		VRAM_READ_FROM_MASK(VRAM_H);
		VRAM_READ_FROM_MASK(VRAM_I);

		return value;
	}
}

template <typename T>
void
vram_write_bbg(NDS *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->gpu.bbg_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->gpu.bbg_bank[index >> 1];
		VRAM_WRITE_FROM_MASK(VRAM_C);
		VRAM_WRITE_FROM_MASK(VRAM_H);
		VRAM_WRITE_FROM_MASK(VRAM_I);
	}
}

template <typename T>
T
vram_read_aobj(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 15;
	u8 *p = nds->gpu.aobj_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->gpu.aobj_bank[index];
		T value = 0;

		VRAM_READ_FROM_MASK(VRAM_A);
		VRAM_READ_FROM_MASK(VRAM_B);
		VRAM_READ_FROM_MASK(VRAM_E);
		VRAM_READ_FROM_MASK(VRAM_F);
		VRAM_READ_FROM_MASK(VRAM_G);

		return value;
	}
}

template <typename T>
void
vram_write_aobj(NDS *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 15;
	u8 *p = nds->gpu.aobj_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->gpu.aobj_bank[index];
		VRAM_WRITE_FROM_MASK(VRAM_A);
		VRAM_WRITE_FROM_MASK(VRAM_B);
		VRAM_WRITE_FROM_MASK(VRAM_E);
		VRAM_WRITE_FROM_MASK(VRAM_F);
		VRAM_WRITE_FROM_MASK(VRAM_G);
	}
}

template <typename T>
T
vram_read_bobj(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->gpu.bobj_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->gpu.bobj_bank;
		T value = 0;

		VRAM_READ_FROM_MASK(VRAM_D);
		VRAM_READ_FROM_MASK(VRAM_I);

		return value;
	}
}

template <typename T>
void
vram_write_bobj(NDS *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->gpu.bobj_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->gpu.bobj_bank;
		VRAM_WRITE_FROM_MASK(VRAM_D);
		VRAM_WRITE_FROM_MASK(VRAM_I);
	}
}

template <typename T>
T
vram_read_lcdc(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 63;
	u8 *p = nds->gpu.lcdc_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		return 0;
	}
}

template <typename T>
void
vram_write_lcdc(NDS *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 63;
	u8 *p = nds->gpu.lcdc_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	}
}

template <typename T>
T
vram_read(NDS *nds, u32 addr)
{
	switch (addr >> 21 & 0x7) {
	case 0:
		return vram_read_abg<T>(nds, addr);
	case 1:
		return vram_read_bbg<T>(nds, addr);
	case 2:
		return vram_read_aobj<T>(nds, addr);
	case 3:
		return vram_read_bobj<T>(nds, addr);
	case 4:
		return vram_read_lcdc<T>(nds, addr);
	default:
		return 0;
	}
}

template <typename T>
void
vram_write(NDS *nds, u32 addr, T value)
{
	switch (addr >> 21 & 0x7) {
	case 0:
		vram_write_abg<T>(nds, addr, value);
		break;
	case 1:
		vram_write_bbg<T>(nds, addr, value);
		break;
	case 2:
		vram_write_aobj<T>(nds, addr, value);
		break;
	case 3:
		vram_write_bobj<T>(nds, addr, value);
		break;
	case 4:
		vram_write_lcdc<T>(nds, addr, value);
		break;
	}
}

template <typename T>
T
vram_arm7_read(NDS *nds, u32 offset)
{
	u32 index = offset >> 17 & 1;
	u8 *p = nds->gpu.arm7_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x1FFFF);
	} else {
		T value = 0;
		u16 mask = nds->gpu.arm7_bank[index];
		VRAM_READ_FROM_MASK(VRAM_C);
		VRAM_READ_FROM_MASK(VRAM_D);

		return value;
	}
}

template <typename T>
void
vram_arm7_write(NDS *nds, u32 offset, T value)
{
	u32 index = offset >> 17 & 1;
	u8 *p = nds->gpu.arm7_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x1FFFF, value);
	} else {
		u16 mask = nds->gpu.arm7_bank[index];
		VRAM_WRITE_FROM_MASK(VRAM_C);
		VRAM_WRITE_FROM_MASK(VRAM_D);
	}
}

} // namespace twice

#endif
