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

u32
Gpu2D::read32(u8 offset)
{
	switch (offset) {
	case 0x8:
		return (u32)bg_cnt[1] << 16 | bg_cnt[0];
	case 0xC:
		return (u32)bg_cnt[3] << 16 | bg_cnt[2];
	}

	fprintf(stderr, "2d engine read 32 at offset %02X\n", offset);
	return 0;
}

u16
Gpu2D::read16(u8 offset)
{
	switch (offset) {
	case 0x8:
		return bg_cnt[0];
	case 0xA:
		return bg_cnt[1];
	case 0xC:
		return bg_cnt[2];
	case 0xE:
		return bg_cnt[3];
	case 0x48:
		return winin;
	case 0x4A:
		return winout;
	case 0x50:
		return bldcnt;
	case 0x52:
		return bldalpha;
	}

	fprintf(stderr, "2d engine read 16 at offset %02X\n", offset);
	return 0;
}

void
Gpu2D::write32(u8 offset, u32 value)
{
	if (!enabled) {
		return;
	}

	switch (offset) {
	case 0x28:
		bg_x[0] = value;
		return;
	case 0x2C:
		bg_y[0] = value;
		return;
	case 0x38:
		bg_x[1] = value;
		return;
	case 0x3C:
		bg_y[1] = value;
		return;
	case 0x4C:
		mosaic = value;
		return;
	case 0x54:
		bldy = value;
		return;
	}

	write16(offset, value);
	write16(offset + 2, value >> 16);
}

void
Gpu2D::write16(u8 offset, u16 value)
{
	if (!enabled) {
		return;
	}

	switch (offset) {
	case 0x8:
		bg_cnt[0] = value;
		return;
	case 0xA:
		bg_cnt[1] = value;
		return;
	case 0xC:
		bg_cnt[2] = value;
		return;
	case 0xE:
		bg_cnt[3] = value;
		return;
	case 0x10:
		bg_hofs[0] = value;
		return;
	case 0x12:
		bg_vofs[0] = value;
		return;
	case 0x14:
		bg_hofs[1] = value;
		return;
	case 0x16:
		bg_vofs[1] = value;
		return;
	case 0x18:
		bg_hofs[2] = value;
		return;
	case 0x1A:
		bg_vofs[2] = value;
		return;
	case 0x1C:
		bg_hofs[3] = value;
		return;
	case 0x1E:
		bg_vofs[3] = value;
		return;
	case 0x20:
		bg_pa[0] = value;
		return;
	case 0x22:
		bg_pb[0] = value;
		return;
	case 0x24:
		bg_pc[0] = value;
		return;
	case 0x26:
		bg_pd[0] = value;
		return;
	case 0x30:
		bg_pa[1] = value;
		return;
	case 0x32:
		bg_pb[1] = value;
		return;
	case 0x34:
		bg_pc[1] = value;
		return;
	case 0x36:
		bg_pd[1] = value;
		return;
	case 0x40:
		win_h[0] = value;
		return;
	case 0x42:
		win_h[1] = value;
		return;
	case 0x44:
		win_v[0] = value;
		return;
	case 0x46:
		win_v[1] = value;
		return;
	case 0x48:
		winin = value & 0x3F3F;
		return;
	case 0x4A:
		winout = value & 0x3F3F;
		return;
	case 0x4C:
		mosaic = value;
		return;
	case 0x50:
		bldcnt = value & 0x3FFF;
		return;
	case 0x52:
		bldalpha = value & 0x1F1F;
		return;
	case 0x54:
		bldy = value;
		return;
	}

	fprintf(stderr, "2d engine write 16 to offset %02X\n", offset);
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
