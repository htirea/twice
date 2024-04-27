#ifndef TWICE_GPU_VRAM_H
#define TWICE_GPU_VRAM_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct nds_ctx;

enum {
	VRAM_A,
	VRAM_B,
	VRAM_C,
	VRAM_D,
	VRAM_E,
	VRAM_F,
	VRAM_G,
	VRAM_H,
	VRAM_I,
	VRAM_NUM_BANKS,
};

enum : u32 {
	VRAM_A_SIZE = 128_KiB,
	VRAM_A_MASK = 128_KiB - 1,
	VRAM_B_SIZE = 128_KiB,
	VRAM_B_MASK = 128_KiB - 1,
	VRAM_C_SIZE = 128_KiB,
	VRAM_C_MASK = 128_KiB - 1,
	VRAM_D_SIZE = 128_KiB,
	VRAM_D_MASK = 128_KiB - 1,
	VRAM_E_SIZE = 64_KiB,
	VRAM_E_MASK = 64_KiB - 1,
	VRAM_F_SIZE = 16_KiB,
	VRAM_F_MASK = 16_KiB - 1,
	VRAM_G_SIZE = 16_KiB,
	VRAM_G_MASK = 16_KiB - 1,
	VRAM_H_SIZE = 32_KiB,
	VRAM_H_MASK = 32_KiB - 1,
	VRAM_I_SIZE = 16_KiB,
	VRAM_I_MASK = 16_KiB - 1,
};

enum : u32 {
	VRAM_TEXTURE_SIZE = 512_KiB,
	VRAM_TEXTURE_MASK = 512_KiB - 1,
	VRAM_TEXTURE_PALETTE_SIZE = 128_KiB,
	VRAM_TEXTURE_PALETTE_MASK = 128_KiB - 1,
};

struct gpu_vram {
	u8 vram_a[VRAM_A_SIZE]{};
	u8 vram_b[VRAM_B_SIZE]{};
	u8 vram_c[VRAM_C_SIZE]{};
	u8 vram_d[VRAM_D_SIZE]{};
	u8 vram_e[VRAM_E_SIZE]{};
	u8 vram_f[VRAM_F_SIZE]{};
	u8 vram_g[VRAM_G_SIZE]{};
	u8 vram_h[VRAM_H_SIZE]{};
	u8 vram_i[VRAM_I_SIZE]{};

	u8 *bank_to_base_ptr[VRAM_NUM_BANKS]{ vram_a, vram_b, vram_c, vram_d,
		vram_e, vram_f, vram_g, vram_h, vram_i };

	u8 bank_to_page_mask[VRAM_NUM_BANKS]{ 7, 7, 7, 7, 3, 0, 0, 1, 0 };

	bool bank_mapped[VRAM_NUM_BANKS]{};

	u8 *lcdc_pt[64]{};
	u8 *abg_pt[32]{};
	u16 abg_bank[32]{};
	u8 *aobj_pt[16]{};
	u16 aobj_bank[16]{};
	u8 *bbg_pt[8]{};
	u16 bbg_bank[4]{};
	u8 *bobj_pt[8]{};
	u16 bobj_bank{};
	u8 *arm7_pt[2]{};
	u16 arm7_bank[2]{};
	u8 *abg_palette_pt[2]{};
	u16 abg_palette_bank[2]{};
	u8 *bbg_palette_pt{};
	u8 *aobj_palette_pt{};
	u16 aobj_palette_bank{};
	u8 *bobj_palette_pt{};
	u8 *texture_pt[4]{};
	u16 texture_bank[4]{};
	/* last two unused */
	u8 *texture_palette_pt[8]{};
	u16 texture_palette_bank[8]{};
	u8 texture_fast[VRAM_TEXTURE_SIZE]{};
	u8 texture_palette_fast[VRAM_TEXTURE_PALETTE_SIZE]{};
	bool texture_changed{};
	bool texture_palette_changed{};

	u8 vramcnt[VRAM_NUM_BANKS]{};
};

template <typename T>
T
vram_bank_read(gpu_vram *vram, u32 offset, int bank)
{
	switch (bank) {
	case VRAM_A:
		return readarr<T>(vram->vram_a, offset & VRAM_A_MASK);
	case VRAM_B:
		return readarr<T>(vram->vram_b, offset & VRAM_B_MASK);
	case VRAM_C:
		return readarr<T>(vram->vram_c, offset & VRAM_C_MASK);
	case VRAM_D:
		return readarr<T>(vram->vram_d, offset & VRAM_D_MASK);
	case VRAM_E:
		return readarr<T>(vram->vram_e, offset & VRAM_E_MASK);
	case VRAM_F:
		return readarr<T>(vram->vram_f, offset & VRAM_F_MASK);
	case VRAM_G:
		return readarr<T>(vram->vram_g, offset & VRAM_G_MASK);
	case VRAM_H:
		return readarr<T>(vram->vram_h, offset & VRAM_H_MASK);
	case VRAM_I:
		return readarr<T>(vram->vram_i, offset & VRAM_I_MASK);
	default:
		return 0;
	}
}

template <typename T>
void
vram_bank_write(gpu_vram *vram, u32 offset, T value, int bank)
{
	switch (bank) {
	case VRAM_A:
		writearr<T>(vram->vram_a, offset & VRAM_A_MASK, value);
		break;
	case VRAM_B:
		writearr<T>(vram->vram_b, offset & VRAM_B_MASK, value);
		break;
	case VRAM_C:
		writearr<T>(vram->vram_c, offset & VRAM_C_MASK, value);
		break;
	case VRAM_D:
		writearr<T>(vram->vram_d, offset & VRAM_D_MASK, value);
		break;
	case VRAM_E:
		writearr<T>(vram->vram_e, offset & VRAM_E_MASK, value);
		break;
	case VRAM_F:
		writearr<T>(vram->vram_f, offset & VRAM_F_MASK, value);
		break;
	case VRAM_G:
		writearr<T>(vram->vram_g, offset & VRAM_G_MASK, value);
		break;
	case VRAM_H:
		writearr<T>(vram->vram_h, offset & VRAM_H_MASK, value);
		break;
	case VRAM_I:
		writearr<T>(vram->vram_i, offset & VRAM_I_MASK, value);
	}
}

void setup_fast_texture_vram(nds_ctx *nds);

} // namespace twice

#endif
