#ifndef TWICE_GPU2D_H
#define TWICE_GPU2D_H

#include "common/types.h"
#include "common/util.h"

#include "nds/gpu/color.h"

namespace twice {

struct nds_ctx;

struct gpu_2d_engine {
	u32 dispcnt{};
	u16 bg_cnt[4]{};
	u16 bg_hofs[4]{};
	u16 bg_vofs[4]{};
	u16 bg_pa[2]{};
	u16 bg_pb[2]{};
	u16 bg_pc[2]{};
	u16 bg_pd[2]{};
	s32 bg_ref_x[2]{};
	s32 bg_ref_y[2]{};
	s32 bg_ref_x_nm[2]{};
	s32 bg_ref_y_nm[2]{};
	u32 bg_ref_x_latch[2]{};
	u32 bg_ref_y_latch[2]{};
	bool bg_ref_x_reload[2]{};
	bool bg_ref_y_reload[2]{};
	u16 win_h[2]{};
	u16 win_v[2]{};
	u16 winin{};
	u16 winout{};
	u16 mosaic{};
	u16 bldcnt{};
	u16 bldalpha{};
	u16 bldy{};
	u16 master_bright{};
	s32 mosaic_countup{};
	s32 mosaic_countdown{};
	s32 obj_mosaic_countup{};
	s32 obj_mosaic_countdown{};
	bool display_capture{};
	u32 dispcapcnt{};
	bool window_y_in_range[2]{};
	bool window_enabled[3]{};
	bool window_any_enabled{};
	u8 window_bits[4]{};
	std::array<u8, 256> active_window{};
	std::array<u8, 256> window_bits_line{};
	std::array<color_u, 256> bg_line[4]{};
	std::array<color_u, 256> obj_line{};
	std::array<color4, 256> gfx_line{};
	std::array<color4, 256> output_line{};

	/*
	 * attributes
	 * 0		effect top
	 * 1		force blend
	 * 2		from 3d
	 * 3		obj mosaic
	 * 8		effect bottom
	 * 12-15	obj alpha
	 * 16-22	obj num
	 * 24-25	bg num
	 * 26		bg
	 * 28-29	priority
	 * 30		backdrop
	 * 31		transparent
	 */

	std::array<u32, 256> bg_attr[4]{};
	std::array<u32, 256> obj_attr{};
	u32 palette_offset{};

	u32 *fb{};
	bool enabled{};
	nds_ctx *nds{};
	int engineid{};
};

void gpu2d_init(nds_ctx *nds);
u8 gpu_2d_read8(gpu_2d_engine *gpu, u8 offset);
u16 gpu_2d_read16(gpu_2d_engine *gpu, u8 offset);
u32 gpu_2d_read32(gpu_2d_engine *gpu, u8 offset);
void gpu_2d_write8(gpu_2d_engine *gpu, u8 offset, u8 value);
void gpu_2d_write16(gpu_2d_engine *gpu, u8 offset, u16 value);
void gpu_2d_write32(gpu_2d_engine *gpu, u8 offset, u32 value);
void gpu_on_scanline_start(nds_ctx *nds);

} // namespace twice

#endif
