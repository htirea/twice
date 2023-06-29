#ifndef TWICE_GPU_H
#define TWICE_GPU_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct nds_ctx;

struct gpu_2d_engine {
	gpu_2d_engine(nds_ctx *nds, int engineid);

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

	u32 *fb{};

	struct pixel {
		u32 color{};
		u8 priority{};
	};

	pixel buffer_top[256]{};
	pixel buffer_bottom[256]{};
	pixel obj_buffer[256]{};

	bool enabled{};
	nds_ctx *nds{};
	int engineid{};
};

u32 gpu_2d_read32(gpu_2d_engine *gpu, u8 offset);
u16 gpu_2d_read16(gpu_2d_engine *gpu, u8 offset);
void gpu_2d_write32(gpu_2d_engine *gpu, u8 offset, u32 value);
void gpu_2d_write16(gpu_2d_engine *gpu, u8 offset, u16 value);

void gpu_on_scanline_start(nds_ctx *nds);

} // namespace twice

#endif
