#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/gpu/color.h"
#include "nds/mem/vram.h"

#include "common/logger.h"
#include "libtwice/exception.h"

namespace twice {

gpu_2d_engine::gpu_2d_engine(nds_ctx *nds, int engineid)
	: nds(nds),
	  engineid(engineid)
{
}

u32
gpu_2d_read32(gpu_2d_engine *gpu, u8 offset)
{
	switch (offset) {
	case 0x8:
		return (u32)gpu->bg_cnt[1] << 16 | gpu->bg_cnt[0];
	case 0xC:
		return (u32)gpu->bg_cnt[3] << 16 | gpu->bg_cnt[2];
	case 0x48:
		return (u32)gpu->winout << 16 | gpu->winin;
	}

	LOG("2d engine read 32 at offset %02X\n", offset);
	return 0;
}

u16
gpu_2d_read16(gpu_2d_engine *gpu, u8 offset)
{
	switch (offset) {
	case 0x8:
		return gpu->bg_cnt[0];
	case 0xA:
		return gpu->bg_cnt[1];
	case 0xC:
		return gpu->bg_cnt[2];
	case 0xE:
		return gpu->bg_cnt[3];
	case 0x48:
		return gpu->winin;
	case 0x4A:
		return gpu->winout;
	case 0x50:
		return gpu->bldcnt;
	case 0x52:
		return gpu->bldalpha;
	}

	LOG("2d engine read 16 at offset %02X\n", offset);
	return 0;
}

u8
gpu_2d_read8(gpu_2d_engine *, u8 offset)
{
	LOG("2d engine read 8 at offset %02X\n", offset);
	return 0;
}

void
gpu_2d_write32(gpu_2d_engine *gpu, u8 offset, u32 value)
{
	if (!gpu->enabled) {
		return;
	}

	switch (offset) {
	case 0x28:
		gpu->bg_ref_x_latch[0] = value;
		gpu->bg_ref_x_reload[0] = true;
		return;
	case 0x2C:
		gpu->bg_ref_y_latch[0] = value;
		gpu->bg_ref_y_reload[0] = true;
		return;
	case 0x38:
		gpu->bg_ref_x_latch[1] = value;
		gpu->bg_ref_x_reload[1] = true;
		return;
	case 0x3C:
		gpu->bg_ref_y_latch[1] = value;
		gpu->bg_ref_y_reload[1] = true;
		return;
	case 0x4C:
		gpu->mosaic = value;
		return;
	case 0x54:
		gpu->bldy = value;
		return;
	}

	gpu_2d_write16(gpu, offset, value);
	gpu_2d_write16(gpu, offset + 2, value >> 16);
}

void
gpu_2d_write16(gpu_2d_engine *gpu, u8 offset, u16 value)
{
	if (!gpu->enabled) {
		return;
	}

	switch (offset) {
	case 0x8:
		gpu->bg_cnt[0] = value;
		return;
	case 0xA:
		gpu->bg_cnt[1] = value;
		return;
	case 0xC:
		gpu->bg_cnt[2] = value;
		return;
	case 0xE:
		gpu->bg_cnt[3] = value;
		return;
	case 0x10:
		gpu->bg_hofs[0] = value;
		return;
	case 0x12:
		gpu->bg_vofs[0] = value;
		return;
	case 0x14:
		gpu->bg_hofs[1] = value;
		return;
	case 0x16:
		gpu->bg_vofs[1] = value;
		return;
	case 0x18:
		gpu->bg_hofs[2] = value;
		return;
	case 0x1A:
		gpu->bg_vofs[2] = value;
		return;
	case 0x1C:
		gpu->bg_hofs[3] = value;
		return;
	case 0x1E:
		gpu->bg_vofs[3] = value;
		return;
	case 0x20:
		gpu->bg_pa[0] = value;
		return;
	case 0x22:
		gpu->bg_pb[0] = value;
		return;
	case 0x24:
		gpu->bg_pc[0] = value;
		return;
	case 0x26:
		gpu->bg_pd[0] = value;
		return;
	case 0x30:
		gpu->bg_pa[1] = value;
		return;
	case 0x32:
		gpu->bg_pb[1] = value;
		return;
	case 0x34:
		gpu->bg_pc[1] = value;
		return;
	case 0x36:
		gpu->bg_pd[1] = value;
		return;
	case 0x40:
		gpu->win_h[0] = value;
		return;
	case 0x42:
		gpu->win_h[1] = value;
		return;
	case 0x44:
		gpu->win_v[0] = value;
		return;
	case 0x46:
		gpu->win_v[1] = value;
		return;
	case 0x48:
		gpu->winin = value & 0x3F3F;
		return;
	case 0x4A:
		gpu->winout = value & 0x3F3F;
		return;
	case 0x4C:
		gpu->mosaic = value;
		return;
	case 0x50:
		gpu->bldcnt = value & 0x3FFF;
		return;
	case 0x52:
		gpu->bldalpha = value & 0x1F1F;
		return;
	case 0x54:
		gpu->bldy = value;
		return;
	}

	LOG("2d engine write 16 to offset %02X\n", offset);
}

void
gpu_2d_write8(gpu_2d_engine *gpu, u8 offset, u8 value)
{
	switch (offset) {
	case 0x40:
		gpu->win_h[0] = (gpu->win_h[0] & 0xFF00) | value;
		return;
	case 0x41:
		gpu->win_h[0] = (gpu->win_h[0] & 0x00FF) | (u16)value << 8;
		return;
	case 0x42:
		gpu->win_h[1] = (gpu->win_h[1] & 0xFF00) | value;
		return;
	case 0x43:
		gpu->win_h[1] = (gpu->win_h[1] & 0x00FF) | (u16)value << 8;
		return;
	case 0x44:
		gpu->win_v[0] = (gpu->win_v[0] & 0xFF00) | value;
		return;
	case 0x45:
		gpu->win_v[0] = (gpu->win_v[0] & 0x00FF) | (u16)value << 8;
		return;
	case 0x46:
		gpu->win_v[1] = (gpu->win_v[0] & 0xFF00) | value;
		return;
	case 0x47:
		gpu->win_v[1] = (gpu->win_v[0] & 0x00FF) | (u16)value << 8;
		return;
	}

	LOG("2d engine write 8 to offset %08X\n", offset);
}

static void draw_scanline(gpu_2d_engine *gpu, u16 scanline);
static void graphics_display_scanline(gpu_2d_engine *gpu);
static void vram_display_scanline(gpu_2d_engine *gpu);
static void render_sprites(gpu_2d_engine *gpu);
static void render_text_bg(gpu_2d_engine *gpu, int bg);
static void render_affine_bg(gpu_2d_engine *gpu, int bg);
static void render_extended_bg(gpu_2d_engine *gpu, int bg);
static void render_large_bitmap_bg(gpu_2d_engine *gpu);
static void render_3d(gpu_2d_engine *gpu);

static void draw_bg_pixel(
		gpu_2d_engine *gpu, u32 fb_x, u16 color, u8 priority);
static void draw_obj_pixel(
		gpu_2d_engine *gpu, u32 fb_x, u16 color, u8 priority);
static u16 bg_get_color_256(gpu_2d_engine *gpu, u32 color_num);
static u16 obj_get_color_256(gpu_2d_engine *gpu, u32 color_num);
static u16 bg_get_color_extended(
		gpu_2d_engine *gpu, u32 slot, u32 palette_num, u32 color_num);
static u16 obj_get_color_extended(
		gpu_2d_engine *gpu, u32 palette_num, u32 color_num);
static u16 bg_get_color_16(gpu_2d_engine *gpu, u32 palette_num, u32 color_num);
static u16 obj_get_color_16(
		gpu_2d_engine *gpu, u32 palette_num, u32 color_num);

static void
reload_bg_ref_xy(gpu_2d_engine *gpu, bool force_reload)
{
	for (int i = 0; i < 2; i++) {
		if (gpu->bg_ref_x_reload[i] || force_reload) {
			gpu->bg_ref_x[i] = SEXT<28>(gpu->bg_ref_x_latch[i]);
			gpu->bg_ref_x_reload[i] = false;
		}
		if (gpu->bg_ref_y_reload[i] || force_reload) {
			gpu->bg_ref_y[i] = SEXT<28>(gpu->bg_ref_y_latch[i]);
			gpu->bg_ref_y_reload[i] = false;
		}
	}
}

static void
increment_bg_ref_xy(gpu_2d_engine *gpu)
{
	for (int i = 0; i < 2; i++) {
		gpu->bg_ref_x[i] += (s16)gpu->bg_pb[i];
		gpu->bg_ref_y[i] += (s16)gpu->bg_pd[i];
	}
}

void
gpu_on_scanline_start(nds_ctx *nds)
{
	if (nds->vcount < 192) {
		bool force_reload = nds->vcount == 0;
		reload_bg_ref_xy(&nds->gpu2d[0], force_reload);
		reload_bg_ref_xy(&nds->gpu2d[1], force_reload);

		draw_scanline(&nds->gpu2d[0], nds->vcount);
		draw_scanline(&nds->gpu2d[1], nds->vcount);

		increment_bg_ref_xy(&nds->gpu2d[0]);
		increment_bg_ref_xy(&nds->gpu2d[1]);
	}
}

static void
fb_fill_white(u32 *fb, u16 scanline)
{
	u32 start = scanline * 256;
	u32 end = start + 256;

	for (u32 i = start; i < end; i++) {
		fb[i] = 0xFFFFFFFF;
	}
}

static void
draw_scanline(gpu_2d_engine *gpu, u16 scanline)
{
	if (!gpu->enabled) {
		fb_fill_white(gpu->fb, scanline);
		return;
	}

	switch (gpu->dispcnt >> 16 & 0x3) {
	case 0:
		fb_fill_white(gpu->fb, scanline);
		break;
	case 1:
		graphics_display_scanline(gpu);
		break;
	case 2:
		if (gpu->engineid == 0) {
			vram_display_scanline(gpu);
		} else {
			throw twice_error("engine B display mode 2");
		}
		break;
	case 3:
		throw twice_error("display mode 3 not implemented");
	}
}

static void
clear_buffers(gpu_2d_engine *gpu)
{
	u16 backdrop_color = bg_get_color_256(gpu, 0);
	for (u32 i = 0; i < 256; i++) {
		gpu->buffer_top[i].color = backdrop_color;
		gpu->buffer_top[i].priority = 4;
		gpu->buffer_bottom[i] = gpu->buffer_top[i];

		gpu->obj_buffer[i].color = 0;
		gpu->obj_buffer[i].priority = 7;
	}
}

static void
graphics_display_scanline(gpu_2d_engine *gpu)
{
	auto& dispcnt = gpu->dispcnt;

	clear_buffers(gpu);

	if (dispcnt & BIT(12)) {
		render_sprites(gpu);
	}

	switch (dispcnt & 0x7) {
	case 0:
		if (dispcnt & BIT(8)) render_text_bg(gpu, 0);
		if (dispcnt & BIT(9)) render_text_bg(gpu, 1);
		if (dispcnt & BIT(10)) render_text_bg(gpu, 2);
		if (dispcnt & BIT(11)) render_text_bg(gpu, 3);
		break;
	case 1:
		if (dispcnt & BIT(8)) render_text_bg(gpu, 0);
		if (dispcnt & BIT(9)) render_text_bg(gpu, 1);
		if (dispcnt & BIT(10)) render_text_bg(gpu, 2);
		if (dispcnt & BIT(11)) render_affine_bg(gpu, 3);
		break;
	case 2:
		if (dispcnt & BIT(8)) render_text_bg(gpu, 0);
		if (dispcnt & BIT(9)) render_text_bg(gpu, 1);
		if (dispcnt & BIT(10)) render_affine_bg(gpu, 2);
		if (dispcnt & BIT(11)) render_affine_bg(gpu, 3);
		break;
	case 3:
		if (dispcnt & BIT(8)) render_text_bg(gpu, 0);
		if (dispcnt & BIT(9)) render_text_bg(gpu, 1);
		if (dispcnt & BIT(10)) render_text_bg(gpu, 2);
		if (dispcnt & BIT(11)) render_extended_bg(gpu, 3);
		break;
	case 4:
		if (dispcnt & BIT(8)) render_text_bg(gpu, 0);
		if (dispcnt & BIT(9)) render_text_bg(gpu, 1);
		if (dispcnt & BIT(10)) render_affine_bg(gpu, 2);
		if (dispcnt & BIT(11)) render_extended_bg(gpu, 3);
		break;
	case 5:
		if (dispcnt & BIT(8)) render_text_bg(gpu, 0);
		if (dispcnt & BIT(9)) render_text_bg(gpu, 1);
		if (dispcnt & BIT(10)) render_extended_bg(gpu, 2);
		if (dispcnt & BIT(11)) render_extended_bg(gpu, 3);
		break;
	case 6:
		if (gpu->engineid == 1) {
			throw twice_error("bg mode 6 invalid for engine B");
		}
		if (dispcnt & BIT(8)) render_3d(gpu);
		if (dispcnt & BIT(10)) render_large_bitmap_bg(gpu);
		break;
	case 7:
		throw twice_error("bg mode 7 invalid");
	}

	for (u32 i = 0; i < 256; i++) {
		auto& obj = gpu->obj_buffer[i];

		if (obj.priority <= gpu->buffer_top[i].priority) {
			gpu->buffer_bottom[i] = gpu->buffer_top[i];
			gpu->buffer_top[i] = obj;
		} else if (obj.priority <= gpu->buffer_bottom[i].priority) {
			gpu->buffer_bottom[i] = obj;
		}
	}

	/* TODO: figure out what format to store pixel colors in */

	for (u32 i = 0; i < 256; i++) {
		gpu->fb[gpu->nds->vcount * 256 + i] =
				bgr555_to_bgr888(gpu->buffer_top[i].color);
	}
}

template <typename T>
static T
read_bg_data(gpu_2d_engine *gpu, u32 offset)
{
	if (gpu->engineid == 0) {
		return vram_read_abg<T>(gpu->nds, offset);
	} else {
		return vram_read_bbg<T>(gpu->nds, offset);
	}
}

template <typename T>
static T
read_bg_data(gpu_2d_engine *gpu, u32 base, u32 w, u32 x, u32 y)
{
	u32 offset = base + sizeof(T) * w * y + sizeof(T) * x;
	return read_bg_data<T>(gpu, offset);
}

static u64
fetch_char_row(gpu_2d_engine *gpu, u16 se, u32 char_base, u32 bg_y,
		bool color_256)
{
	u64 char_row;

	u16 char_name = se & 0x3FF;

	u32 py = bg_y % 8;
	if (se & BIT(11)) {
		py = 7 - py;
	}

	if (color_256) {
		char_row = read_bg_data<u64>(
				gpu, char_base + 64 * char_name + 8 * py);
		if (se & BIT(10)) {
			char_row = byteswap64(char_row);
		}
	} else {
		char_row = read_bg_data<u32>(
				gpu, char_base + 32 * char_name + 4 * py);
		if (se & BIT(10)) {
			char_row = byteswap32(char_row);
			char_row = (char_row & 0x0F0F0F0F) << 4 |
			           (char_row & 0xF0F0F0F0) >> 4;
		}
	}

	return char_row;
}

static void
render_text_bg(gpu_2d_engine *gpu, int bg)
{
	if (gpu->engineid == 0 && bg == 0 && (gpu->dispcnt & BIT(3))) {
		render_3d(gpu);
		return;
	}

	u32 text_bg_widths[] = { 256, 512, 256, 512 };
	u32 text_bg_heights[] = { 256, 256, 512, 512 };

	u32 bg_priority = gpu->bg_cnt[bg] & 0x3;
	u32 bg_size_bits = gpu->bg_cnt[bg] >> 14 & 0x3;
	u32 bg_w = text_bg_widths[bg_size_bits];
	u32 bg_h = text_bg_heights[bg_size_bits];
	u32 bg_x = gpu->bg_hofs[bg] & (bg_w - 1);
	u32 bg_y = (gpu->bg_vofs[bg] + gpu->nds->vcount) & (bg_h - 1);

	u32 screen = (bg_y / 256 * 2) + (bg_x / 256);
	if (bg_size_bits == 2) {
		screen /= 2;
	}

	u32 screen_base = (gpu->bg_cnt[bg] >> 8 & 0x1F) * 0x800;
	if (gpu->engineid == 0) {
		screen_base += (gpu->dispcnt >> 27 & 0x7) * 0x10000;
	}

	u32 char_base = (gpu->bg_cnt[bg] >> 2 & 0xF) * 0x4000;
	if (gpu->engineid == 0) {
		char_base += (gpu->dispcnt >> 24 & 0x7) * 0x10000;
	}

	u32 px = bg_x % 8;
	u32 se_x = bg_x % 256 / 8;
	u32 se_y = bg_y % 256 / 8;

	bool color_256 = gpu->bg_cnt[bg] & BIT(7);
	u64 char_row;
	u32 palette_num;

	bool extended_palettes = gpu->dispcnt & BIT(30);
	u32 slot = bg;
	if (gpu->bg_cnt[bg] & BIT(13)) {
		slot |= 2;
	}

	for (u32 i = 0; i < 256;) {
		if (px == 0 || i == 0) {
			u32 se_offset = screen_base + 0x800 * screen +
			                64 * se_y + 2 * se_x;
			u16 se = read_bg_data<u16>(gpu, se_offset);
			char_row = fetch_char_row(
					gpu, se, char_base, bg_y, color_256);
			char_row >>= (color_256 ? 8 : 4) * px;
			palette_num = se >> 12;
		}

		if (color_256) {
			for (; px < 8 && i < 256; px++, i++, char_row >>= 8) {
				u32 offset = char_row & 0xFF;
				if (offset == 0) continue;

				u16 color;
				if (extended_palettes) {
					color = bg_get_color_extended(gpu,
							slot, palette_num,
							offset);
				} else {
					color = bg_get_color_256(gpu, offset);
				}
				draw_bg_pixel(gpu, i, color, bg_priority);
			}
		} else {
			for (; px < 8 && i < 256; px++, i++, char_row >>= 4) {
				u32 offset = char_row & 0xF;
				if (offset != 0) {
					u16 color = bg_get_color_16(gpu,
							palette_num, offset);
					draw_bg_pixel(gpu, i, color,
							bg_priority);
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

static void
render_affine_bg(gpu_2d_engine *gpu, int bg)
{
	u32 bg_priority = gpu->bg_cnt[bg] & 0x3;
	u32 bg_w = 128 << (gpu->bg_cnt[bg] >> 14 & 0x3);
	u32 bg_h = bg_w;

	u32 screen_base = (gpu->bg_cnt[bg] >> 8 & 0x1F) * 0x800;
	if (gpu->engineid == 0) {
		screen_base += (gpu->dispcnt >> 27 & 0x7) * 0x10000;
	}

	u32 char_base = (gpu->bg_cnt[bg] >> 2 & 0xF) * 0x4000;
	if (gpu->engineid == 0) {
		char_base += (gpu->dispcnt >> 24 & 0x7) * 0x10000;
	}

	s32 ref_x = gpu->bg_ref_x[bg - 2];
	s32 ref_y = gpu->bg_ref_y[bg - 2];
	s16 pa = gpu->bg_pa[bg - 2];
	s16 pc = gpu->bg_pc[bg - 2];

	bool wrap_bg = gpu->bg_cnt[bg] & BIT(13);

	for (int i = 0; i < 256; i++, ref_x += pa, ref_y += pc) {
		u32 bg_x = ref_x >> 8;
		u32 bg_y = ref_y >> 8;

		if (wrap_bg) {
			bg_x = bg_x & (bg_w - 1);
			bg_y = bg_y & (bg_h - 1);
		} else if ((s32)bg_x < 0 || bg_x >= bg_w || (s32)bg_y < 0 ||
				bg_y >= bg_h) {
			continue;
		}

		u32 se_x = bg_x / 8;
		u32 se_y = bg_y / 8;
		u32 px = bg_x % 8;
		u32 py = bg_y % 8;

		u32 se_offset = screen_base + bg_w / 8 * se_y + se_x;
		u8 se = read_bg_data<u8>(gpu, se_offset);
		/* char_name = se */
		u8 color_num = read_bg_data<u8>(
				gpu, char_base + 64 * se + 8 * py + px);
		if (color_num != 0) {
			u16 color = bg_get_color_256(gpu, color_num);
			draw_bg_pixel(gpu, i, color, bg_priority);
		}
	}
}

static void
render_extended_text_bg(gpu_2d_engine *gpu, int bg)
{
	u32 bg_priority = gpu->bg_cnt[bg] & 0x3;
	u32 bg_w = 128 << (gpu->bg_cnt[bg] >> 14 & 0x3);
	u32 bg_h = bg_w;

	u32 screen_base = (gpu->bg_cnt[bg] >> 8 & 0x1F) * 0x800;
	if (gpu->engineid == 0) {
		screen_base += (gpu->dispcnt >> 27 & 0x7) * 0x10000;
	}

	u32 char_base = (gpu->bg_cnt[bg] >> 2 & 0xF) * 0x4000;
	if (gpu->engineid == 0) {
		char_base += (gpu->dispcnt >> 24 & 0x7) * 0x10000;
	}

	s32 ref_x = gpu->bg_ref_x[bg - 2];
	s32 ref_y = gpu->bg_ref_y[bg - 2];
	s16 pa = gpu->bg_pa[bg - 2];
	s16 pc = gpu->bg_pc[bg - 2];

	bool wrap_bg = gpu->bg_cnt[bg] & BIT(13);

	bool extended_palettes = gpu->dispcnt & BIT(30);
	u32 slot = bg;
	if (gpu->bg_cnt[bg] & BIT(13)) {
		slot |= 2;
	}

	for (int i = 0; i < 256; i++, ref_x += pa, ref_y += pc) {
		u32 bg_x = ref_x >> 8;
		u32 bg_y = ref_y >> 8;

		if (wrap_bg) {
			bg_x = bg_x & (bg_w - 1);
			bg_y = bg_y & (bg_h - 1);
		} else if ((s32)bg_x < 0 || bg_x >= bg_w || (s32)bg_y < 0 ||
				bg_y >= bg_h) {
			continue;
		}

		u32 se_x = bg_x / 8;
		u32 se_y = bg_y / 8;
		u32 px = bg_x % 8;
		u32 py = bg_y % 8;

		u32 se_offset = screen_base + bg_w / 8 * 2 * se_y + 2 * se_x;
		u16 se = read_bg_data<u16>(gpu, se_offset);
		if (se & BIT(11)) {
			py = 7 - py;
		}
		if (se & BIT(10)) {
			px = 7 - px;
		}

		u16 char_name = se & 0x3FF;
		u8 color_num = read_bg_data<u8>(
				gpu, char_base + 64 * char_name + 8 * py + px);
		if (color_num != 0) {
			u16 color;
			if (extended_palettes) {
				u32 palette_num = se >> 12;
				color = bg_get_color_extended(gpu, slot,
						palette_num, color_num);
			} else {
				color = bg_get_color_256(gpu, color_num);
			}
			draw_bg_pixel(gpu, i, color, bg_priority);
		}
	}
}

static void
render_extended_bitmap_bg(gpu_2d_engine *gpu, int bg, bool direct_color)
{
	u32 bitmap_bg_widths[] = { 128, 256, 512, 512 };
	u32 bitmap_bg_heights[] = { 128, 256, 256, 512 };

	u32 bg_priority = gpu->bg_cnt[bg] & 0x3;
	u32 bg_size_bits = gpu->bg_cnt[bg] >> 14 & 0x3;
	u32 bg_w = bitmap_bg_widths[bg_size_bits];
	u32 bg_h = bitmap_bg_heights[bg_size_bits];
	u32 screen_base = (gpu->bg_cnt[bg] >> 8 & 0x1F) * 0x4000;

	s32 ref_x = gpu->bg_ref_x[bg - 2];
	s32 ref_y = gpu->bg_ref_y[bg - 2];
	s16 pa = gpu->bg_pa[bg - 2];
	s16 pc = gpu->bg_pc[bg - 2];

	bool wrap_bg = gpu->bg_cnt[bg] & BIT(13);

	for (int i = 0; i < 256; i++, ref_x += pa, ref_y += pc) {
		u32 bg_x = ref_x >> 8;
		u32 bg_y = ref_y >> 8;

		if (wrap_bg) {
			bg_x = bg_x & (bg_w - 1);
			bg_y = bg_y & (bg_h - 1);
		} else if ((s32)bg_x < 0 || bg_x >= bg_w || (s32)bg_y < 0 ||
				bg_y >= bg_h) {
			continue;
		}

		if (direct_color) {
			u16 color = read_bg_data<u16>(
					gpu, screen_base, bg_w, bg_x, bg_y);
			if (color & BIT(15)) {
				draw_bg_pixel(gpu, i, color, bg_priority);
			}
		} else {
			u8 color_num = read_bg_data<u8>(
					gpu, screen_base, bg_w, bg_x, bg_y);
			if (color_num != 0) {
				u16 color = bg_get_color_256(gpu, color_num);
				draw_bg_pixel(gpu, i, color, bg_priority);
			}
		}
	}
}

static void
render_extended_bg(gpu_2d_engine *gpu, int bg)
{
	if (!(gpu->bg_cnt[bg] & BIT(7))) {
		render_extended_text_bg(gpu, bg);
	} else {
		bool direct_color = gpu->bg_cnt[bg] & BIT(2);
		render_extended_bitmap_bg(gpu, bg, direct_color);
	}
}

static void
render_large_bitmap_bg(gpu_2d_engine *gpu)
{
	int bg = 2;

	u32 bg_priority = gpu->bg_cnt[bg] & 0x3;
	u32 bg_size_bits = gpu->bg_cnt[bg] >> 14 & 0x3;
	u32 bg_w;
	u32 bg_h;
	switch (bg_size_bits) {
	case 0:
		bg_w = 512;
		bg_h = 1024;
		break;
	case 1:
		bg_w = 1024;
		bg_h = 512;
		break;
	default:
		throw twice_error("large bitmap bg invalid size");
	}

	s32 ref_x = gpu->bg_ref_x[bg - 2];
	s32 ref_y = gpu->bg_ref_y[bg - 2];
	s16 pa = gpu->bg_pa[bg - 2];
	s16 pc = gpu->bg_pc[bg - 2];

	bool wrap_bg = gpu->bg_cnt[bg] & BIT(13);

	for (int i = 0; i < 256; i++, ref_x += pa, ref_y += pc) {
		u32 bg_x = ref_x >> 8;
		u32 bg_y = ref_y >> 8;

		if (wrap_bg) {
			bg_x = bg_x & (bg_w - 1);
			bg_y = bg_y & (bg_h - 1);
		} else if ((s32)bg_x < 0 || bg_x >= bg_w || (s32)bg_y < 0 ||
				bg_y >= bg_h) {
			continue;
		}

		u8 color_num = vram_read<u8>(gpu->nds, bg_w * bg_y + bg_x);
		if (color_num != 0) {
			u16 color = bg_get_color_256(gpu, color_num);
			draw_bg_pixel(gpu, i, color, bg_priority);
		}
	}
}

static void
render_3d(gpu_2d_engine *)
{
}

struct obj_data {
	u16 attr0;
	u16 attr1;
	u16 attr2;
	u32 w;
	u32 h;
};

static void
read_oam_attrs(gpu_2d_engine *gpu, int obj_num, obj_data *obj)
{
	u32 offset = obj_num << 3;
	if (gpu->engineid == 1) {
		offset += 0x400;
	}

	u64 data = readarr<u64>(gpu->nds->oam, offset);
	obj->attr0 = data;
	obj->attr1 = data >> 16;
	obj->attr2 = data >> 32;
}

static void
get_obj_size(obj_data *obj, u32 *obj_w, u32 *obj_h)
{
	u32 shape_bits = obj->attr0 >> 14 & 3;
	u32 size_bits = obj->attr1 >> 14 & 3;

	if (shape_bits == 3) {
		throw twice_error("invalid obj shape bits");
	}

	u32 widths[][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 },
		{ 8, 8, 16, 32 } };
	u32 heights[][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 },
		{ 16, 32, 32, 64 } };

	*obj_w = widths[shape_bits][size_bits];
	*obj_h = heights[shape_bits][size_bits];
}

template <typename T>
static T
read_obj_data(gpu_2d_engine *gpu, u32 offset)
{
	if (gpu->engineid == 0) {
		return vram_read_aobj<T>(gpu->nds, offset);
	} else {
		return vram_read_bobj<T>(gpu->nds, offset);
	}
}

static u64
fetch_obj_char_row(gpu_2d_engine *gpu, u32 tile_offset, u32 py, bool hflip,
		bool color_256)
{
	u64 char_row;

	if (color_256) {
		char_row = read_obj_data<u64>(gpu, tile_offset + 8 * py);
		if (hflip) {
			char_row = byteswap64(char_row);
		}
	} else {
		char_row = read_obj_data<u32>(gpu, tile_offset + 4 * py);
		if (hflip) {
			char_row = byteswap32(char_row);
			char_row = (char_row & 0x0F0F0F0F) << 4 |
			           (char_row & 0xF0F0F0F0) >> 4;
		}
	}

	return char_row;
}

static void
render_normal_sprite(gpu_2d_engine *gpu, obj_data *obj)
{
	u32 obj_w, obj_h;
	get_obj_size(obj, &obj_w, &obj_h);

	u32 obj_attr_y = obj->attr0 & 0xFF;
	u32 obj_y = (gpu->nds->vcount - obj_attr_y) & 0xFF;
	if (obj_y >= obj_h) {
		return;
	}
	if (obj->attr1 & BIT(13)) {
		obj_y = obj_h - 1 - obj_y;
	}
	bool hflip = obj->attr1 & BIT(12);

	u32 obj_attr_x = obj->attr1 & 0x1FF;
	u32 obj_x, draw_x, draw_x_end;
	if (obj_attr_x < 256) {
		obj_x = 0;
		draw_x = obj_attr_x;
		draw_x_end = std::min(obj_attr_x + obj_w, (u32)256);
	} else {
		obj_x = 512 - obj_attr_x;
		if (obj_x >= obj_w) {
			return;
		}
		draw_x = 0;
		draw_x_end = obj_w - obj_x;
	}

	u32 priority = obj->attr2 >> 10 & 3;
	u32 palette_num = obj->attr2 >> 12;

	bool color_256 = obj->attr0 & BIT(13);

	u32 px = obj_x % 8;
	u32 tx = obj_x / 8;
	if (hflip) {
		tx = (obj_w / 8) - 1 - tx;
	}
	u32 py = obj_y % 8;
	u32 ty = obj_y / 8;

	u32 obj_char_name = obj->attr2 & 0x3FF;
	bool map_1d = gpu->dispcnt & BIT(4);
	bool extended_palettes = gpu->dispcnt & BIT(31);

	u32 tile_offset;
	if (map_1d) {
		tile_offset = obj_char_name << 5 << (gpu->dispcnt >> 20 & 3);
		if (color_256) {
			tile_offset += (obj_w / 8) * 64 * ty + 64 * tx;
		} else {
			tile_offset += (obj_w / 8) * 32 * ty + 32 * tx;
		}
	} else {
		if (color_256) {
			tile_offset = 32 * (obj_char_name & ~1) + 1024 * ty +
			              64 * tx;
		} else {
			tile_offset = 32 * obj_char_name + 1024 * ty + 32 * tx;
		}
	}

	u64 char_row = 0;

	for (; draw_x != draw_x_end;) {
		if (px == 0 || draw_x == 0) {
			char_row = fetch_obj_char_row(gpu, tile_offset, py,
					hflip, color_256);
			char_row >>= (color_256 ? 8 : 4) * px;
		}

		if (color_256) {
			for (; px < 8 && draw_x != draw_x_end;
					px++, draw_x++, char_row >>= 8) {
				u32 offset = char_row & 0xFF;
				if (offset == 0) continue;

				u16 color;
				if (extended_palettes) {
					color = obj_get_color_extended(gpu,
							palette_num, offset);
				} else {
					color = obj_get_color_256(gpu, offset);
				}
				draw_obj_pixel(gpu, draw_x, color, priority);
			}
		} else {
			for (; px < 8 && draw_x != draw_x_end;
					px++, draw_x++, char_row >>= 4) {
				u32 offset = char_row & 0xF;
				if (offset != 0) {
					u16 color = obj_get_color_16(gpu,
							palette_num, offset);
					draw_obj_pixel(gpu, draw_x, color,
							priority);
				}
			}
		}

		px = 0;
		if (color_256) {
			tile_offset = hflip ? tile_offset - 64
			                    : tile_offset + 64;
		} else {
			tile_offset = hflip ? tile_offset - 32
			                    : tile_offset + 32;
		}
	}
}

static void
render_bitmap_sprite(gpu_2d_engine *, obj_data *)
{
	throw twice_error("bitmap sprite");
}

struct affine_sprite_data {
	u32 draw_start;
	u32 draw_end;
	s32 ref_x;
	s32 ref_y;
	s16 pa;
	s16 pc;
};

static int
init_affine_sprite_data(
		gpu_2d_engine *gpu, obj_data *obj, affine_sprite_data *affine)
{
	get_obj_size(obj, &obj->w, &obj->h);

	u32 box_w = obj->w;
	u32 box_h = obj->h;
	if (obj->attr0 & BIT(9)) {
		box_w <<= 1;
		box_h <<= 1;
	}
	u32 half_box_w = box_w >> 1;
	u32 half_box_h = box_h >> 1;

	u32 obj_attr_y = obj->attr0 & 0xFF;
	u32 box_y = (gpu->nds->vcount - obj_attr_y) & 0xFF;
	if (box_y >= box_h) {
		return 1;
	}

	u32 obj_attr_x = obj->attr1 & 0x1FF;
	u32 box_x;
	if (obj_attr_x < 256) {
		box_x = 0;
		affine->draw_start = obj_attr_x;
		affine->draw_end = std::min(obj_attr_x + box_w, (u32)256);
	} else {
		box_x = 512 - obj_attr_x;
		if (box_x >= box_w) {
			return 1;
		}
		affine->draw_start = 0;
		affine->draw_end = box_w - box_x;
	}

	u32 affine_index = obj->attr1 >> 9 & 0x1F;
	u32 affine_base_addr = 0x20 * affine_index + 6;
	if (gpu->engineid == 1) {
		affine_base_addr += 0x400;
	}
	s16 pa = readarr<u16>(gpu->nds->oam, affine_base_addr);
	s16 pb = readarr<u16>(gpu->nds->oam, affine_base_addr + 8);
	s16 pc = readarr<u16>(gpu->nds->oam, affine_base_addr + 16);
	s16 pd = readarr<u16>(gpu->nds->oam, affine_base_addr + 24);

	affine->ref_x = (s32)(box_y - half_box_h) * pb +
	                (s32)(box_x - half_box_w) * pa + (obj->w >> 1 << 8);
	affine->ref_y = (s32)(box_y - half_box_h) * pd +
	                (s32)(box_x - half_box_w) * pc + (obj->h >> 1 << 8);
	affine->pa = pa;
	affine->pc = pc;

	return 0;
}

static void
render_affine_sprite(gpu_2d_engine *gpu, obj_data *obj)
{
	affine_sprite_data affine;
	if (init_affine_sprite_data(gpu, obj, &affine)) {
		return;
	}

	u32 priority = obj->attr2 >> 10 & 3;
	u32 palette_num = obj->attr2 >> 12;
	u32 obj_char_name = obj->attr2 & 0x3FF;

	bool map_1d = gpu->dispcnt & BIT(4);
	bool extended_palettes = gpu->dispcnt & BIT(31);
	bool color_256 = obj->attr0 & BIT(13);

	for (u32 i = affine.draw_start, end = affine.draw_end; i != end; i++,
		 affine.ref_x += affine.pa, affine.ref_y += affine.pc) {
		s32 obj_x = affine.ref_x >> 8;
		s32 obj_y = affine.ref_y >> 8;

		if (obj_x < 0 || (u32)obj_x >= obj->w || obj_y < 0 ||
				(u32)obj_y >= obj->h) {
			continue;
		}

		u32 px = obj_x & 7;
		u32 tx = obj_x >> 3;
		u32 py = obj_y & 7;
		u32 ty = obj_y >> 3;

		u32 tile_offset;
		if (map_1d) {
			tile_offset = obj_char_name
			              << 5 << (gpu->dispcnt >> 20 & 3);
			if (color_256) {
				tile_offset += (obj->w / 8) * 64 * ty +
				               64 * tx + 8 * py + px;
			} else {
				tile_offset += (obj->w / 8) * 32 * ty +
				               32 * tx + 4 * py + px / 2;
			}
		} else {
			if (color_256) {
				tile_offset = 32 * (obj_char_name & ~1) +
				              1024 * ty + 64 * tx + 8 * py +
				              px;
			} else {
				tile_offset = 32 * obj_char_name + 1024 * ty +
				              32 * tx + 4 * py + px / 2;
			}
		}

		u8 offset = read_obj_data<u8>(gpu, tile_offset);

		if (color_256) {
			if (offset != 0) {
				u16 color;
				if (extended_palettes) {
					color = obj_get_color_extended(gpu,
							palette_num, offset);
				} else {
					color = obj_get_color_256(gpu, offset);
				}
				draw_obj_pixel(gpu, i, color, priority);
			}
		} else {
			offset = px & 1 ? offset >> 4 : offset & 0xF;
			if (offset != 0) {
				u16 color = obj_get_color_16(
						gpu, palette_num, offset);
				draw_obj_pixel(gpu, i, color, priority);
			}
		}
	}
}

static void
render_affine_bitmap_sprite(gpu_2d_engine *gpu, obj_data *obj)
{
	affine_sprite_data affine;
	if (init_affine_sprite_data(gpu, obj, &affine)) {
		return;
	}

	u32 priority = obj->attr2 >> 10 & 3;
	u32 obj_char_name = obj->attr2 & 0x3FF;

	bool map_1d = gpu->dispcnt & BIT(6);
	u32 start_offset;
	if (map_1d) {
		start_offset = obj_char_name << 7 << (gpu->dispcnt >> 22 & 1);
	} else {
		u32 mask_x = gpu->dispcnt & BIT(5) ? 0x1F : 0xF;
		start_offset = 0x10 * (obj_char_name & mask_x) +
		               0x80 * (obj_char_name & ~mask_x);
	}
	u32 width_2d = 128 << (gpu->dispcnt >> 5 & 1);

	for (u32 i = affine.draw_start, end = affine.draw_end; i != end; i++,
		 affine.ref_x += affine.pa, affine.ref_y += affine.pc) {
		u32 obj_x = affine.ref_x >> 8;
		u32 obj_y = affine.ref_y >> 8;

		if ((s32)obj_x < 0 || obj_x >= obj->w || (s32)obj_y < 0 ||
				obj_y >= obj->h) {
			continue;
		}

		u32 offset = start_offset;
		if (map_1d) {
			offset += obj->w * 2 * obj_y + 2 * obj_x;
		} else {
			offset += width_2d * 2 * obj_y + 2 * obj_x;
		}

		u16 color = read_obj_data<u16>(gpu, offset);
		if (color & BIT(15)) {
			draw_obj_pixel(gpu, i, color, priority);
		}
	}
}

static void
render_sprites(gpu_2d_engine *gpu)
{
	for (int i = 0; i < 128; i++) {
		obj_data obj;
		read_oam_attrs(gpu, i, &obj);

		if ((obj.attr0 >> 8 & 3) == 2) {
			continue;
		}

		bool is_affine = obj.attr0 & BIT(8);
		bool is_bitmap = (obj.attr0 >> 10 & 3) == 3;

		if (is_affine && is_bitmap) {
			render_affine_bitmap_sprite(gpu, &obj);
		} else if (is_affine) {
			render_affine_sprite(gpu, &obj);
		} else if (is_bitmap) {
			render_bitmap_sprite(gpu, &obj);
		} else {
			render_normal_sprite(gpu, &obj);
		}
	}
}

static void
vram_display_scanline(gpu_2d_engine *gpu)
{
	u32 start = gpu->nds->vcount * 256;
	u32 end = start + 256;

	u32 offset = 0x20000 * (gpu->dispcnt >> 18 & 0x3);

	for (u32 i = start; i < end; i++) {
		gpu->fb[i] = bgr555_to_bgr888(
				vram_read_lcdc<u16>(gpu->nds, offset + i * 2));
	}
}

static void
draw_bg_pixel(gpu_2d_engine *gpu, u32 fb_x, u16 color, u8 priority)
{
	auto& top = gpu->buffer_top[fb_x];
	auto& bottom = gpu->buffer_bottom[fb_x];

	if (priority < top.priority) {
		bottom = top;
		top.color = color;
		top.priority = priority;
	} else if (priority < bottom.priority) {
		bottom.color = color;
		bottom.priority = priority;
	}
}

static void
draw_obj_pixel(gpu_2d_engine *gpu, u32 fb_x, u16 color, u8 priority)
{
	auto& top = gpu->obj_buffer[fb_x];

	if (priority < top.priority) {
		top.color = color;
		top.priority = priority;
	}
}

static u16
bg_get_color_256(gpu_2d_engine *gpu, u32 color_num)
{
	u32 offset = 2 * color_num;
	if (gpu->engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(gpu->nds->palette, offset);
}

static u16
obj_get_color_256(gpu_2d_engine *gpu, u32 color_num)
{
	u32 offset = 0x200 + 2 * color_num;
	if (gpu->engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(gpu->nds->palette, offset);
}

static u16
bg_get_color_extended(
		gpu_2d_engine *gpu, u32 slot, u32 palette_num, u32 color_num)
{
	u32 offset = 0x2000 * slot + 512 * palette_num + 2 * color_num;
	if (gpu->engineid == 0) {
		return vram_read_abg_palette<u16>(gpu->nds, offset);
	} else {
		return vram_read_bbg_palette<u16>(gpu->nds, offset);
	}
}

static u16
obj_get_color_extended(gpu_2d_engine *gpu, u32 palette_num, u32 color_num)
{
	u32 offset = 512 * palette_num + 2 * color_num;
	if (gpu->engineid == 0) {
		return vram_read_aobj_palette<u16>(gpu->nds, offset);
	} else {
		return vram_read_bobj_palette<u16>(gpu->nds, offset);
	}
}

static u16
bg_get_color_16(gpu_2d_engine *gpu, u32 palette_num, u32 color_num)
{
	u32 offset = 32 * palette_num + 2 * color_num;
	if (gpu->engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(gpu->nds->palette, offset);
}

static u16
obj_get_color_16(gpu_2d_engine *gpu, u32 palette_num, u32 color_num)
{
	u32 offset = 0x200 + 32 * palette_num + 2 * color_num;
	if (gpu->engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(gpu->nds->palette, offset);
}

} // namespace twice
