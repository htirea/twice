#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/mem/vram.h"

#include "libtwice/exception.h"

namespace twice {

static const u32 text_bg_widths[] = { 256, 512, 256, 512 };
static const u32 text_bg_heights[] = { 256, 256, 512, 512 };

u32
BGR555_TO_BGR888(u16 color)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;

	r = (r * 527 + 23) >> 6;
	g = (g * 527 + 23) >> 6;
	b = (b * 527 + 23) >> 6;

	return b << 16 | g << 8 | r;
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
		bg_ref_x[0] = value;
		return;
	case 0x2C:
		bg_ref_y[0] = value;
		return;
	case 0x38:
		bg_ref_x[1] = value;
		return;
	case 0x3C:
		bg_ref_y[1] = value;
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
		return;
	}

	switch (dispcnt >> 16 & 0x3) {
	case 0:
		fb_fill_white(fb, scanline);
		break;
	case 1:
		graphics_display_scanline();
		break;
	case 2:
		if (engineid == 0) {
			vram_display_scanline();
		} else {
			throw TwiceError("engine B display mode 2");
		}
		break;
	case 3:
		throw TwiceError("display mode 3 not implemented");
	}
}

void
Gpu2D::set_backdrop()
{
	u16 backdrop_color = readarr<u16>(nds->palette, 0);
	for (u32 i = 0; i < NDS_SCREEN_W; i++) {
		bg_buffer_top[i].color = backdrop_color;
		bg_buffer_top[i].priority = 4;
		bg_buffer_bottom[i] = bg_buffer_top[i];
	}
}

void
Gpu2D::graphics_display_scanline()
{
	set_backdrop();

	switch (dispcnt & 0x7) {
	case 0:
		if (dispcnt & BIT(11)) render_text_bg(3);
		if (dispcnt & BIT(10)) render_text_bg(2);
		if (dispcnt & BIT(9)) render_text_bg(1);
		if (dispcnt & BIT(8)) render_text_bg(0);
		break;
	case 6:
		if (engineid == 1) {
			throw TwiceError("bg mode 6 invalid for engine B");
		}
		throw TwiceError("bg mode 6 not implemented");
		break;
	case 7:
		throw TwiceError("bg mode 7 invalid");
	default:
		throw TwiceError("bg mode not implemented");
	}

	/* TODO: figure out what format to store pixel colors in */

	for (u32 i = 0; i < NDS_SCREEN_W; i++) {
		fb[nds->vcount * NDS_FB_W + i] =
				BGR555_TO_BGR888(bg_buffer_top[i].color);
	}
}

void
Gpu2D::render_text_bg(int bg)
{
	if (engineid == 0 && (dispcnt & BIT(3))) {
		throw TwiceError("3D not implemented");
	}

	u32 bg_priority = bg_cnt[bg] & 0x3;

	u32 bg_size_bits = bg_cnt[bg] >> 14 & 0x3;
	u32 bg_w = text_bg_widths[bg_size_bits];
	u32 bg_h = text_bg_heights[bg_size_bits];

	u32 bg_x = bg_hofs[bg] & (bg_w - 1);
	u32 bg_y = bg_vofs[bg] + nds->vcount & (bg_h - 1);

	u32 screen = (bg_y / 256 * 2) + (bg_x / 256);
	if (bg_size_bits == 2) {
		screen /= 2;
	}

	u32 screen_base = (bg_cnt[bg] >> 8 & 0x1F) * 0x800;
	if (engineid == 0) {
		screen_base += (dispcnt >> 27 & 0x7) * 0x10000;
	}

	u32 char_base = (bg_cnt[bg] >> 2 & 0xF) * 0x4000;
	if (engineid == 0) {
		char_base += (dispcnt >> 24 & 0x7) * 0x10000;
	}

	int px = bg_x % 8;
	u32 se_x = bg_x % 256 / 8;
	u32 se_y = bg_y % 256 / 8;

	bool color_256 = bg_cnt[bg] & BIT(7);
	u64 char_row;
	u32 palette_bank;

	for (u32 i = 0; i < 256;) {
		if (px == 0 || i == 0) {
			u16 se = get_screen_entry(
					screen, screen_base, se_x, se_y);
			char_row = fetch_char_row(
					se, char_base, bg_y, color_256);

			if (color_256) {
				char_row >>= 8 * px;
			} else {
				char_row >>= 4 * px;
				palette_bank = se >> 12;
			}
		}

		if (color_256) {
			for (; px < 8 && i < 256; px++, i++) {
				u32 offset = char_row & 0xFF;
				char_row >>= 8;
				if (offset != 0) {
					u16 color = get_palette_color_256(
							offset);
					draw_text_bg_pixel(
							i, color, bg_priority);
				}
			}
		} else {
			for (; px < 8 && i < 256; px++, i++) {
				u32 offset = char_row & 0xF;
				char_row >>= 4;
				if (offset != 0) {
					u16 color = get_palette_color_16(
							palette_bank, offset);
					draw_text_bg_pixel(
							i, color, bg_priority);
				}
			}
		}

		px = 0;
		se_x += 1;
		if (se_x >= 32) {
			se_x = 0;
			if (bg_w > 256) {
				screen ^= 1;
			}
		}
	}
}

u64
Gpu2D::fetch_char_row(u16 se, u32 char_base, u32 bg_y, bool color_256)
{
	u64 char_row;

	u16 char_name = se & 0x3FF;

	int py = bg_y % 8;
	if (se & BIT(11)) {
		py = 7 - py;
	}

	if (color_256) {
		char_row = get_char_row_256(char_base, char_name, py);
		if (se & BIT(10)) {
			char_row = __builtin_bswap64(char_row);
		}
	} else {
		char_row = get_char_row_16(char_base, char_name, py);
		if (se & BIT(10)) {
			char_row = __builtin_bswap32(char_row);
		}
	}

	return char_row;
}

void
Gpu2D::draw_text_bg_pixel(u32 fb_x, u16 color, u8 priority)
{
	auto& top = bg_buffer_top[fb_x];
	auto& bottom = bg_buffer_bottom[fb_x];

	if (priority <= top.priority) {
		bottom = top;
		top.color = color;
		top.priority = priority;
	}
}

void
Gpu2D::vram_display_scanline()
{
	u32 start = nds->vcount * NDS_FB_W;
	u32 end = start + NDS_FB_W;

	u32 offset = 0x20000 * (dispcnt >> 18 & 0x3);

	for (u32 i = start; i < end; i++) {
		fb[i] = BGR555_TO_BGR888(
				vram_read_lcdc<u16>(nds, offset + i * 2));
	}
}

u16
Gpu2D::get_screen_entry(u32 screen, u32 base, u32 x, u32 y)
{
	u32 offset = base + 0x800 * screen + 0x40 * y + 2 * x;
	if (engineid == 0) {
		return vram_read_abg<u16>(nds, offset);
	} else {
		return vram_read_bbg<u16>(nds, offset);
	}
}

u64
Gpu2D::get_char_row_256(u32 base, u32 char_name, u32 y)
{
	u32 offset = base + 64 * char_name + 8 * y;
	if (engineid == 0) {
		return vram_read_abg<u64>(nds, offset);
	} else {
		return vram_read_bbg<u64>(nds, offset);
	}
}

u32
Gpu2D::get_char_row_16(u32 base, u32 char_name, u32 y)
{
	u32 offset = base + 32 * char_name + 4 * y;
	if (engineid == 0) {
		return vram_read_abg<u32>(nds, offset);
	} else {
		return vram_read_bbg<u32>(nds, offset);
	}
}

u16
Gpu2D::get_palette_color_256(u32 color_num)
{
	u32 offset = 2 * color_num;
	if (engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(nds->palette, offset);
}

u16
Gpu2D::get_palette_color_16(u32 palette_num, u32 color_num)
{
	u32 offset = 32 * palette_num + 2 * color_num;
	if (engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(nds->palette, offset);
}

} // namespace twice
