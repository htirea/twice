#ifndef TWICE_GPU_H
#define TWICE_GPU_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct NDS;

struct Gpu2D {
	Gpu2D(NDS *nds, int engineid);

	u32 dispcnt{};
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
