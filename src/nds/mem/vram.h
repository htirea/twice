#ifndef TWICE_MEM_VRAM_H
#define TWICE_MEM_VRAM_H

#include "nds/nds.h"

#include "common/util.h"

#define VRAM_READ_FROM_MASK(bank_)                                            \
	if (mask & BIT(bank_)) {                                              \
		value |= vram_bank_read<T, bank_>(&nds->vram, offset);        \
	}

#define VRAM_WRITE_FROM_MASK(bank_)                                           \
	if (mask & BIT(bank_)) {                                              \
		vram_bank_write<T, bank_>(&nds->vram, offset, value);         \
	}

namespace twice {

template <typename T>
T
vram_read_abg(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 31;
	u8 *p = nds->vram.abg_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.abg_bank[index];
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
	u8 *p = nds->vram.abg_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.abg_bank[index];
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
	u8 *p = nds->vram.bbg_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.bbg_bank[index >> 1];
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
	u8 *p = nds->vram.bbg_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.bbg_bank[index >> 1];
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
	u8 *p = nds->vram.aobj_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.aobj_bank[index];
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
	u8 *p = nds->vram.aobj_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.aobj_bank[index];
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
	u8 *p = nds->vram.bobj_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.bobj_bank;
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
	u8 *p = nds->vram.bobj_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.bobj_bank;
		VRAM_WRITE_FROM_MASK(VRAM_D);
		VRAM_WRITE_FROM_MASK(VRAM_I);
	}
}

template <typename T>
T
vram_read_lcdc(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 63;
	u8 *p = nds->vram.lcdc_pt[index];
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
	u8 *p = nds->vram.lcdc_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	}
}

template <typename T>
T
vram_read_abg_palette(NDS *nds, u32 offset)
{
	u32 index = offset >> 14 & 1;
	u8 *p = nds->vram.abg_palette_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.abg_palette_bank[index];
		T value = 0;

		VRAM_READ_FROM_MASK(VRAM_E);
		VRAM_READ_FROM_MASK(VRAM_F);
		VRAM_READ_FROM_MASK(VRAM_G);

		return value;
	}
}

template <typename T>
void
vram_write_abg_palette(NDS *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 1;
	u8 *p = nds->vram.abg_palette_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.abg_palette_bank[index];
		VRAM_WRITE_FROM_MASK(VRAM_E);
		VRAM_WRITE_FROM_MASK(VRAM_F);
		VRAM_WRITE_FROM_MASK(VRAM_G);
	}
}

template <typename T>
T
vram_read_bbg_palette(NDS *nds, u32 offset)
{
	u8 *p = nds->vram.bbg_palette_pt;
	if (p) {
		return readarr<T>(p, offset & 0x7FFF);
	} else {
		return 0;
	}
}

template <typename T>
void
vram_write_bbg_palette(NDS *nds, u32 offset, T value)
{
	u8 *p = nds->vram.bbg_palette_pt;
	if (p) {
		writearr<T>(p, offset & 0x7FFF, value);
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
	u8 *p = nds->vram.arm7_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x1FFFF);
	} else {
		T value = 0;
		u16 mask = nds->vram.arm7_bank[index];
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
	u8 *p = nds->vram.arm7_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x1FFFF, value);
	} else {
		u16 mask = nds->vram.arm7_bank[index];
		VRAM_WRITE_FROM_MASK(VRAM_C);
		VRAM_WRITE_FROM_MASK(VRAM_D);
	}
}

} // namespace twice

#endif
