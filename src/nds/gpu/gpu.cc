#include "nds/nds.h"

#include "libtwice/exception.h"

namespace twice {

u8 *
get_vram_ptr(NDS *nds, int bank, int page)
{
	u8 *base = nds->gpu.bank_to_base_ptr[bank];
	u8 mask = nds->gpu.bank_to_page_mask[bank];

	return base + (page & mask) * 16_KiB;
}

void
map_lcdc(NDS *nds, u8 *base, int start, int len)
{
	for (int i = start; i < start + len; i++, base += 16_KiB) {
		nds->gpu.lcdc_pt[i] = base;
	}
}

void
unmap_lcdc(NDS *nds, int start, int len)
{
	for (int i = start; i < start + len; i++) {
		nds->gpu.lcdc_pt[i] = nullptr;
	}
}

void
map_abg(NDS *nds, int bank, u8 *base, int start, int len)
{
	for (int i = start; i < start + len; i++, base += 16_KiB) {
		auto& mask = nds->gpu.abg_bank[i];

		if (mask == 0) {
			nds->gpu.abg_pt[i] = base;
		} else {
			nds->gpu.abg_pt[i] = nullptr;
		}

		mask |= BIT(bank);
	}
}

void
unmap_abg(NDS *nds, int bank, int start, int len)
{
	for (int i = start; i < start + len; i++) {
		auto& mask = nds->gpu.abg_bank[i];
		mask &= ~BIT(bank);

		if (std::has_single_bit(mask)) {
			int only_bank = std::countr_zero(mask);
			nds->gpu.abg_pt[i] = get_vram_ptr(nds, only_bank, i);
		} else {
			nds->gpu.abg_pt[i] = nullptr;
		}
	}
}

void
map_aobj(NDS *nds, int bank, u8 *base, int start, int len)
{
	for (int i = start; i < start + len; i++, base += 16_KiB) {
		auto& mask = nds->gpu.aobj_bank[i];

		if (mask == 0) {
			nds->gpu.aobj_pt[i] = base;
		} else {
			nds->gpu.aobj_pt[i] = nullptr;
		}

		mask |= BIT(bank);
	}
}

void
unmap_aobj(NDS *nds, int bank, int start, int len)
{
	for (int i = start; i < start + len; i++) {
		auto& mask = nds->gpu.aobj_bank[i];
		mask &= ~BIT(bank);

		if (std::has_single_bit(mask)) {
			int only_bank = std::countr_zero(mask);
			nds->gpu.aobj_pt[i] = get_vram_ptr(nds, only_bank, i);
		} else {
			nds->gpu.aobj_pt[i] = nullptr;
		}
	}
}

void
map_bbg(NDS *nds, int bank, u8 *base, int start, int len)
{
	for (int i = start; i < start + len; i += 2, base += 32_KiB) {
		auto& mask = nds->gpu.bbg_bank[i >> 1];

		if (mask == 0) {
			nds->gpu.bbg_pt[i] = base;
			if (bank == VRAM_I) {
				nds->gpu.bbg_pt[i + 1] = base;
			} else {
				nds->gpu.bbg_pt[i + 1] = base + 16_KiB;
			}
		} else {
			nds->gpu.bbg_pt[i] = nullptr;
			nds->gpu.bbg_pt[i + 1] = nullptr;
		}

		mask |= BIT(bank);
	}
}

void
unmap_bbg(NDS *nds, int bank, int start, int len)
{
	for (int i = start; i < start + len; i += 2) {
		auto& mask = nds->gpu.bbg_bank[i >> 1];
		mask &= ~BIT(bank);

		if (std::has_single_bit(mask)) {
			int only_bank = std::countr_zero(mask);
			u8 *p = get_vram_ptr(nds, only_bank, i);
			nds->gpu.bbg_pt[i] = p;
			if (bank == VRAM_I) {
				nds->gpu.bbg_pt[i + 1] = p;
			} else {
				nds->gpu.bbg_pt[i + 1] = p + 16_KiB;
			}
		} else {
			nds->gpu.bbg_pt[i] = nullptr;
			nds->gpu.bbg_pt[i + 1] = nullptr;
		}
	}
}

void
map_bobj(NDS *nds, int bank, u8 *base)
{
	auto& mask = nds->gpu.bobj_bank;

	if (mask == 0) {
		if (bank == VRAM_D) {
			for (int i = 0; i < 8; i++) {
				nds->gpu.bobj_pt[i] = base + i * 16_KiB;
			}
		} else {
			for (int i = 0; i < 8; i++) {
				nds->gpu.bobj_pt[i] = base;
			}
		}
	} else {
		for (int i = 0; i < 8; i++) {
			nds->gpu.bobj_pt[i] = nullptr;
		}
	}

	mask |= BIT(bank);
}

void
unmap_bobj(NDS *nds, int bank)
{
	auto& mask = nds->gpu.bobj_bank;
	mask &= ~BIT(bank);

	if (mask == BIT(VRAM_D)) {
		for (int i = 0; i < 8; i++) {
			nds->gpu.bobj_pt[i] = nds->gpu.vram_d + i * 16_KiB;
		}
	} else if (mask == BIT(VRAM_I)) {
		for (int i = 0; i < 8; i++) {
			nds->gpu.bobj_pt[i] = nds->gpu.vram_i;
		}
	} else {
		for (int i = 0; i < 8; i++) {
			nds->gpu.bobj_pt[i] = nullptr;
		}
	}
}

void
map_arm7(NDS *nds, int bank, u8 *base, int i)
{
	auto& mask = nds->gpu.arm7_bank[i];

	if (mask == 0) {
		nds->gpu.arm7_pt[i] = base;
	} else {
		nds->gpu.arm7_pt[i] = nullptr;
	}

	mask |= BIT(bank);
}

void
unmap_arm7(NDS *nds, int bank, int i)
{
	auto& mask = nds->gpu.arm7_bank[i];
	mask &= ~BIT(bank);

	if (std::has_single_bit(mask)) {
		int only_bank = std::countr_zero(mask);
		nds->gpu.arm7_pt[i] = nds->gpu.bank_to_base_ptr[only_bank];
	} else {
		nds->gpu.arm7_pt[i] = nullptr;
	}
}

template <int bank>
void
vramcnt_ab_write(NDS *nds, u8 value)
{
	auto& gpu = nds->gpu;

	u8 old = gpu.vramcnt[bank];

	if (value == old) {
		return;
	}

	u8 *base = gpu.bank_to_base_ptr[bank];

	if (gpu.bank_mapped[bank]) {
		u8 mst = old & 0x3;
		u8 ofs = old >> 3 & 0x3;

		switch (mst) {
		case 0:
			unmap_lcdc(nds, bank << 3, 8);
			break;
		case 1:
			unmap_abg(nds, bank, ofs << 3, 8);
			break;
		case 2:
			unmap_aobj(nds, bank, (ofs & 1) << 3, 8);
			break;
		case 3:
			throw TwiceError("unmap vram bank a/b to texture");
		}

		gpu.bank_mapped[bank] = false;
	}

	if (value & 0x80) {
		u8 mst = value & 0x3;
		u8 ofs = value >> 3 & 0x3;

		switch (mst) {
		case 0:
			map_lcdc(nds, base, bank << 3, 8);
			break;
		case 1:
			map_abg(nds, bank, base, ofs << 3, 8);
			break;
		case 2:
			map_aobj(nds, bank, base, (ofs & 1) << 3, 8);
			break;
		case 3:
			throw TwiceError("map vram bank a/b to texture");
		}

		gpu.bank_mapped[bank] = true;
	}

	gpu.vramcnt[bank] = value;
}

template <int bank>
void
vramcnt_cd_write(NDS *nds, u8 value)
{
	auto& gpu = nds->gpu;

	u8 old = gpu.vramcnt[bank];

	if (value == old) {
		return;
	}

	u8 *base = gpu.bank_to_base_ptr[bank];

	if (gpu.bank_mapped[bank]) {
		u8 mst = old & 0x7;
		u8 ofs = old >> 3 & 0x3;

		switch (mst) {
		case 0:
			unmap_lcdc(nds, bank << 3, 8);
			break;
		case 1:
			unmap_abg(nds, bank, ofs << 3, 8);
			break;
		case 2:
			unmap_arm7(nds, bank, ofs & 1);
			break;
		case 3:
			throw TwiceError("unmap vram bank c/d to texture");
		case 4:
			if (bank == VRAM_C) {
				unmap_bbg(nds, bank, 0, 8);
			} else {
				unmap_bobj(nds, bank);
			}
			break;
		}

		nds->vramstat &= ~BIT(bank - VRAM_C);

		gpu.bank_mapped[bank] = false;
	}

	if (value & 0x80) {
		u8 mst = value & 0x7;
		u8 ofs = value >> 3 & 0x3;

		switch (mst) {
		case 0:
			map_lcdc(nds, base, bank << 3, 8);
			break;
		case 1:
			map_abg(nds, bank, base, ofs << 3, 8);
			break;
		case 2:
			map_arm7(nds, bank, base, ofs & 1);
			nds->vramstat |= BIT(bank - VRAM_C);
			break;
		case 3:
			throw TwiceError("map vram bank c/d to texture");
		case 4:
			if (bank == VRAM_C) {
				map_bbg(nds, bank, base, 0, 8);
			} else {
				map_bobj(nds, bank, base);
			}
			break;
		default:
			throw TwiceError("map vram bank c/d invalid mst");
		}

		gpu.bank_mapped[bank] = true;
	}

	gpu.vramcnt[bank] = value;
}

void
vramcnt_e_write(NDS *nds, u8 value)
{
	auto& gpu = nds->gpu;

	u8 old = gpu.vramcnt[VRAM_E];

	if (value == old) {
		return;
	}

	u8 *base = gpu.bank_to_base_ptr[VRAM_E];

	if (gpu.bank_mapped[VRAM_E]) {
		u8 mst = old & 0x7;

		switch (mst) {
		case 0:
			unmap_lcdc(nds, 0x20, 4);
			break;
		case 1:
			unmap_abg(nds, VRAM_E, 0, 4);
			break;
		case 2:
			unmap_aobj(nds, VRAM_E, 0, 4);
			break;
		case 3:
			throw TwiceError("unmap vram bank e tex pal");
		case 4:
			throw TwiceError("unmap vram bank e abg extpal");
		}

		gpu.bank_mapped[VRAM_E] = false;
	}

	if (value & 0x80) {
		u8 mst = value & 0x7;

		switch (mst) {
		case 0:
			map_lcdc(nds, base, 0x20, 4);
			break;
		case 1:
			map_abg(nds, VRAM_E, base, 0, 4);
			break;
		case 2:
			map_aobj(nds, VRAM_E, base, 0, 4);
			break;
		case 3:
			throw TwiceError("map vram bank e tex pal");
		case 4:
			throw TwiceError("map vram bank e abg extpal");
		default:
			throw TwiceError("map vram bank e invalid mst");
		}

		gpu.bank_mapped[VRAM_E] = true;
	}

	gpu.vramcnt[VRAM_E] = value;
}

template <int bank>
void
vramcnt_fg_write(NDS *nds, u8 value)
{
	auto& gpu = nds->gpu;

	u8 old = gpu.vramcnt[bank];

	if (value == old) {
		return;
	}

	u8 *base = gpu.bank_to_base_ptr[bank];

	if (gpu.bank_mapped[bank]) {
		u8 mst = old & 0x7;
		u8 ofs = old >> 3 & 0x3;
		u32 offset = (ofs & 1) | (ofs & 2) << 1;

		switch (mst) {
		case 0:
			map_lcdc(nds, base, 0x24 + bank - VRAM_F, 1);
			break;
		case 1:
			map_abg(nds, bank, base, offset, 1);
			map_abg(nds, bank, base, offset | 2, 1);
			break;
		case 2:
			map_aobj(nds, bank, base, offset, 1);
			map_aobj(nds, bank, base, offset | 2, 1);
			break;
		case 3:
			throw TwiceError("unmap vram bank f/g tex pal");
		case 4:
			throw TwiceError("unmap vram bank f/g abg ext pal");
		case 5:
			throw TwiceError("unmap vram bank f/g aobj ext pal");
		}
		gpu.bank_mapped[bank] = false;
	}

	if (value & 0x80) {
		u8 mst = value & 0x7;
		u8 ofs = value >> 3 & 0x3;
		u32 offset = (ofs & 1) | (ofs & 2) << 1;

		switch (mst) {
		case 0:
			map_lcdc(nds, base, 0x24 + bank - VRAM_F, 1);
			break;
		case 1:
			map_abg(nds, bank, base, offset, 1);
			map_abg(nds, bank, base, offset | 2, 1);
			break;
		case 2:
			map_aobj(nds, bank, base, offset, 1);
			map_aobj(nds, bank, base, offset | 2, 1);
			break;
		case 3:
			throw TwiceError("map vram bank f/g tex pal");
		case 4:
			throw TwiceError("map vram bank f/g abg ext pal");
		case 5:
			throw TwiceError("map vram bank f/g aobj ext pal");
		default:
			throw TwiceError("map vram bank f/g invalid mst");
		}

		gpu.bank_mapped[bank] = true;
	}

	gpu.vramcnt[bank] = value;
}

void
vramcnt_h_write(NDS *nds, u8 value)
{
	auto& gpu = nds->gpu;

	u8 old = gpu.vramcnt[VRAM_H];

	if (value == old) {
		return;
	}

	u8 *base = gpu.bank_to_base_ptr[VRAM_H];

	if (gpu.bank_mapped[VRAM_H]) {
		u8 mst = old & 0x3;

		switch (mst) {
		case 0:
			unmap_lcdc(nds, 0x26, 2);
			break;
		case 1:
			unmap_bbg(nds, VRAM_H, 0, 2);
			unmap_bbg(nds, VRAM_H, 4, 2);
			break;
		case 2:
			throw TwiceError("unmap vram bank h bbg ext pal");
		}

		gpu.bank_mapped[VRAM_H] = false;
	}

	if (value & 0x80) {
		u8 mst = value & 0x3;

		switch (mst) {
		case 0:
			map_lcdc(nds, base, 0x26, 2);
			break;
		case 1:
			map_bbg(nds, VRAM_H, base, 0, 2);
			map_bbg(nds, VRAM_H, base, 4, 2);
			break;
		case 2:
			throw TwiceError("map vram bank h bbg ext pal");
		default:
			throw TwiceError("map vram bank h invalid mst");
		}

		gpu.bank_mapped[VRAM_H] = true;
	}

	gpu.vramcnt[VRAM_H] = value;
}

void
vramcnt_i_write(NDS *nds, u8 value)
{
	auto& gpu = nds->gpu;

	u8 old = gpu.vramcnt[VRAM_I];

	if (value == old) {
		return;
	}

	u8 *base = gpu.bank_to_base_ptr[VRAM_I];

	if (gpu.bank_mapped[VRAM_I]) {
		u8 mst = old & 0x3;

		switch (mst) {
		case 0:
			unmap_lcdc(nds, 0x28, 1);
			break;
		case 1:
			unmap_bbg(nds, VRAM_I, 2, 2);
			unmap_bbg(nds, VRAM_I, 6, 2);
			break;
		case 2:
			unmap_bobj(nds, VRAM_I);
			break;
		case 3:
			throw TwiceError("unmap vram bank i bobj ext pal");
		}

		gpu.bank_mapped[VRAM_I] = false;
	}

	if (value & 0x80) {
		u8 mst = value & 0x3;

		switch (mst) {
		case 0:
			map_lcdc(nds, base, 0x28, 1);
			break;
		case 1:
			map_bbg(nds, VRAM_I, base, 2, 2);
			map_bbg(nds, VRAM_I, base, 6, 2);
			break;
		case 2:
			map_bobj(nds, VRAM_I, base);
			break;
		case 3:
			throw TwiceError("map vram bank i bobj ext pal");
		}

		gpu.bank_mapped[VRAM_I] = true;
	}

	gpu.vramcnt[VRAM_I] = value;
}

void
vramcnt_a_write(NDS *nds, u8 value)
{
	vramcnt_ab_write<VRAM_A>(nds, value);
}

void
vramcnt_b_write(NDS *nds, u8 value)
{
	vramcnt_ab_write<VRAM_B>(nds, value);
}

void
vramcnt_c_write(NDS *nds, u8 value)
{
	vramcnt_cd_write<VRAM_C>(nds, value);
}

void
vramcnt_d_write(NDS *nds, u8 value)
{
	vramcnt_cd_write<VRAM_D>(nds, value);
}

void
vramcnt_f_write(NDS *nds, u8 value)
{
	vramcnt_fg_write<VRAM_F>(nds, value);
}

void
vramcnt_g_write(NDS *nds, u8 value)
{
	vramcnt_fg_write<VRAM_G>(nds, value);
}

} // namespace twice
