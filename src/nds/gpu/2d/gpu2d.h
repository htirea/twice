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
	s32 bg_ref_x_nonmosaic[2]{};
	s32 bg_ref_y_nonmosaic[2]{};
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

	bool display_capture{};
	u32 dispcapcnt{};

	color4 gfx_line[256]{};
	color4 output_line[256]{};

	u32 *fb{};

	struct pixel {
		color_u color;

		u32 priority : 3;
		u32 effect_top : 1;
		u32 effect_bottom : 1;
		u32 force_blend : 1;
		u32 alpha_oam : 4;
		u32 from_3d : 1;
	};

	pixel buffer_top[256]{};
	pixel buffer_bottom[256]{};
	pixel obj_buffer[256]{};

	bool window_y_in_range[2]{};
	bool window_enabled[3]{};
	bool window_any_enabled{};

	struct {
		u8 window{};
	} window_buffer[256];

	struct {
		u8 enable_bits;
	} window[4];

	bool enabled{};
	nds_ctx *nds{};
	int engineid{};
};

void gpu_on_scanline_start(nds_ctx *nds);

u8 gpu_2d_read8(gpu_2d_engine *gpu, u8 offset);
u16 gpu_2d_read16(gpu_2d_engine *gpu, u8 offset);
u32 gpu_2d_read32(gpu_2d_engine *gpu, u8 offset);
void gpu_2d_write8(gpu_2d_engine *gpu, u8 offset, u8 value);
void gpu_2d_write16(gpu_2d_engine *gpu, u8 offset, u16 value);
void gpu_2d_write32(gpu_2d_engine *gpu, u8 offset, u32 value);

} // namespace twice

#endif
