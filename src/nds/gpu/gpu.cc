#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/mem/vram.h"

namespace twice {

u32
ABGR1555_TO_ABGR8888(u16 color)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;
	u8 a = color >> 15;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	a = a * 0xFF;

	return (a << 24) | (b << 16) | (g << 8) | r;
}

void
gpu_on_scanline_start(NDS *nds)
{
	if (nds->vcount < 192) {
		nds->gpu2D[0].draw_scanline(nds->vcount);
	}
}

Gpu2D::Gpu2D(NDS *nds)
	: nds(nds)
{
}

void
Gpu2D::draw_scanline(u16 scanline)
{
	u32 start = scanline * NDS_FB_W;
	u32 end = start + NDS_FB_W;

	for (u32 i = start; i < end; i++) {
		fb[i] = ABGR1555_TO_ABGR8888(vram_read_lcdc<u16>(nds, i * 2));
	}
}

} // namespace twice
