#ifndef TWICE_MEM_VRAM_H
#define TWICE_MEM_VRAM_H

#include "nds/nds.h"

#include "common/util.h"

namespace twice {

template <typename T>
T
vram_read_banks(nds_ctx *nds, u32 offset, u16 banks)
{
	T value = 0;

	for (int i = 0; i < VRAM_NUM_BANKS; i++) {
		if (banks & BIT(i)) {
			value |= vram_bank_read<T>(&nds->vram, offset, i);
		}
	}

	return value;
}

template <typename T>
void
vram_write_banks(nds_ctx *nds, u32 offset, T value, u16 banks)
{
	for (int i = 0; i < VRAM_NUM_BANKS; i++) {
		if (banks & BIT(i)) {
			vram_bank_write<T>(&nds->vram, offset, value, i);
		}
	}
}

template <typename T>
T
vram_read_abg(nds_ctx *nds, u32 offset)
{
	u32 index = offset >> 14 & 31;
	u8 *p = nds->vram.abg_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.abg_bank[index];
		return vram_read_banks<T>(nds, offset, mask);
	}
}

template <typename T>
void
vram_write_abg(nds_ctx *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 31;
	u8 *p = nds->vram.abg_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.abg_bank[index];
		vram_write_banks<T>(nds, offset, value, mask);
	}
}

template <typename T>
T
vram_read_bbg(nds_ctx *nds, u32 offset)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->vram.bbg_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.bbg_bank[index >> 1];
		return vram_read_banks<T>(nds, offset, mask);
	}
}

template <typename T>
void
vram_write_bbg(nds_ctx *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->vram.bbg_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.bbg_bank[index >> 1];
		vram_write_banks<T>(nds, offset, value, mask);
	}
}

template <typename T>
T
vram_read_aobj(nds_ctx *nds, u32 offset)
{
	u32 index = offset >> 14 & 15;
	u8 *p = nds->vram.aobj_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.aobj_bank[index];
		return vram_read_banks<T>(nds, offset, mask);
	}
}

template <typename T>
void
vram_write_aobj(nds_ctx *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 15;
	u8 *p = nds->vram.aobj_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.aobj_bank[index];
		vram_write_banks<T>(nds, offset, value, mask);
	}
}

template <typename T>
T
vram_read_bobj(nds_ctx *nds, u32 offset)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->vram.bobj_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.bobj_bank;
		return vram_read_banks<T>(nds, offset, mask);
	}
}

template <typename T>
void
vram_write_bobj(nds_ctx *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 7;
	u8 *p = nds->vram.bobj_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	} else {
		u16 mask = nds->vram.bobj_bank;
		vram_write_banks<T>(nds, offset, value, mask);
	}
}

template <typename T>
T
vram_read_lcdc(nds_ctx *nds, u32 offset)
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
vram_write_lcdc(nds_ctx *nds, u32 offset, T value)
{
	u32 index = offset >> 14 & 63;
	u8 *p = nds->vram.lcdc_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x3FFF, value);
	}
}

template <typename T>
T
vram_read_abg_palette(nds_ctx *nds, u32 offset)
{
	u32 index = offset >> 14 & 1;
	u8 *p = nds->vram.abg_palette_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x3FFF);
	} else {
		u16 mask = nds->vram.abg_palette_bank[index];
		return vram_read_banks<T>(nds, offset, mask);
	}
}

template <typename T>
T
vram_read_bbg_palette(nds_ctx *nds, u32 offset)
{
	u8 *p = nds->vram.bbg_palette_pt;
	if (p) {
		return readarr<T>(p, offset & 0x7FFF);
	} else {
		return 0;
	}
}

template <typename T>
T
vram_read_aobj_palette(nds_ctx *nds, u32 offset)
{
	u8 *p = nds->vram.aobj_palette_pt;
	if (p) {
		return readarr<T>(p, offset & 0x1FFF);
	} else {
		u16 mask = nds->vram.aobj_palette_bank;
		return vram_read_banks<T>(nds, offset, mask);
	}
}

template <typename T>
T
vram_read_bobj_palette(nds_ctx *nds, u32 offset)
{
	u8 *p = nds->vram.bobj_palette_pt;
	if (p) {
		return readarr<T>(p, offset & 0x1FFF);
	} else {
		return 0;
	}
}

template <typename T>
T
vram_read_texture(nds_ctx *nds, u32 offset)
{
	return readarr<T>(nds->vram.texture_fast, offset & VRAM_TEXTURE_MASK);
}

template <typename T>
T
vram_read_texture_palette(nds_ctx *nds, u32 offset)
{
	return readarr<T>(nds->vram.texture_palette_fast,
			offset & VRAM_TEXTURE_PALETTE_MASK);
}

template <typename T>
T
vram_read(nds_ctx *nds, u32 addr)
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
vram_write(nds_ctx *nds, u32 addr, T value)
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
vram_arm7_read(nds_ctx *nds, u32 offset)
{
	u32 index = offset >> 17 & 1;
	u8 *p = nds->vram.arm7_pt[index];
	if (p) {
		return readarr<T>(p, offset & 0x1FFFF);
	} else {
		u16 mask = nds->vram.arm7_bank[index];
		return vram_read_banks<T>(nds, offset, mask);
	}
}

template <typename T>
void
vram_arm7_write(nds_ctx *nds, u32 offset, T value)
{
	u32 index = offset >> 17 & 1;
	u8 *p = nds->vram.arm7_pt[index];
	if (p) {
		writearr<T>(p, offset & 0x1FFFF, value);
	} else {
		u16 mask = nds->vram.arm7_bank[index];
		vram_write_banks<T>(nds, offset, value, mask);
	}
}

} // namespace twice

#endif
