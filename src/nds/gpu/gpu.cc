#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/mem/vram.h"

#include "libtwice/exception.h"

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
fb_fill_white(u32 *fb, u16 scanline)
{
	u32 start = scanline * NDS_FB_W;
	u32 end = start + NDS_FB_W;

	for (u32 i = start; i < end; i++) {
		fb[i] = 0xFFFFFFFF;
	}
}

void
gpu_on_scanline_start(NDS *nds)
{
	if (nds->vcount < 192) {
		nds->gpu2D[0].draw_scanline(nds->vcount);
		nds->gpu2D[1].draw_scanline(nds->vcount);
	}
}

Gpu2D::Gpu2D(NDS *nds, int engineid)
	: nds(nds),
	  engineid(engineid)
{
}

void
Gpu2D::draw_scanline(u16 scanline)
{
	if (!enabled) {
		fb_fill_white(fb, scanline);
	}

	switch (dispcnt >> 16 & 0x3) {
	case 0:
		fb_fill_white(fb, scanline);
		break;
	case 1:
		throw TwiceError("display mode 1 not implemented");
	case 2:
		if (engineid == 0) {
			vram_display_scanline(scanline);
		} else {
			throw TwiceError("engine B display mode 2");
		}
		break;
	case 3:
		throw TwiceError("display mode 3 not implemented");
	}
}

void
Gpu2D::vram_display_scanline(u16 scanline)
{
	u32 start = scanline * NDS_FB_W;
	u32 end = start + NDS_FB_W;

	u32 offset = 0x20000 * (dispcnt >> 18 & 0x3);

	for (u32 i = start; i < end; i++) {
		fb[i] = ABGR1555_TO_ABGR8888(
				vram_read_lcdc<u16>(nds, offset + i * 2));
	}
}

} // namespace twice
