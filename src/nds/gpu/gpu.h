#ifndef TWICE_GPU_H
#define TWICE_GPU_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct NDS;

struct Gpu2D {
	Gpu2D(NDS *nds, int engineid);

	u32 dispcnt{};
	u16 bg_cnt[4]{};
	u16 bg_hofs[4]{};
	u16 bg_vofs[4]{};
	u16 bg_pa[2]{};
	u16 bg_pb[2]{};
	u16 bg_pc[2]{};
	u16 bg_pd[2]{};
	u32 bg_x[2]{};
	u32 bg_y[2]{};
	u16 win_h[2]{};
	u16 win_v[2]{};
	u16 winin{};
	u16 winout{};
	u16 mosaic{};
	u16 bldcnt{};
	u16 bldalpha{};
	u16 bldy{};

	bool enabled{};
	u32 *fb{};
	NDS *nds{};
	int engineid{};

	u32 read32(u8 offset);
	u16 read16(u8 offset);
	void write32(u8 offset, u32 value);
	void write16(u8 offset, u16 value);

	void draw_scanline(u16 scanline);
	void vram_display_scanline(u16 scanline);
};

void gpu_on_scanline_start(NDS *nds);

} // namespace twice

#endif
