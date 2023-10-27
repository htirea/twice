#include "nds/mem/vram.h"
#include "nds/nds.h"

namespace twice {

static void draw_bg_pixel(gpu_2d_engine *gpu, int bg, u32 fb_x, color_u color,
		u8 priority, bool effect_top, bool effect_bottom,
		bool from_3d);
static void draw_2d_bg_pixel(gpu_2d_engine *gpu, int bg, u32 fb_x, u16 color,
		u8 priority, bool effect_top, bool effect_bottom);
static void draw_3d_bg_pixel(gpu_2d_engine *gpu, u32 fb_x, color4 color,
		u8 priority, bool effect_top, bool effect_bottom);
u16 bg_get_color_256(gpu_2d_engine *gpu, u32 color_num);
static u16 bg_get_color_extended(
		gpu_2d_engine *gpu, u32 slot, u32 palette_num, u32 color_num);
static u16 bg_get_color_16(gpu_2d_engine *gpu, u32 palette_num, u32 color_num);

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

void
render_3d(gpu_2d_engine *gpu)
{
	u32 priority = gpu->bg_cnt[0] & 0x3;
	s32 offset = SEXT<9>(gpu->bg_hofs[0]);

	u32 x, draw_x, draw_x_end;
	if (offset < 0) {
		x = 0;
		draw_x = -offset;
		draw_x_end = 256;
	} else {
		x = offset;
		draw_x = 0;
		draw_x_end = 256 - offset;
	}

	bool effect_top = gpu->bldcnt & BIT(0);
	bool effect_bottom = gpu->bldcnt & BIT(8);

	for (; draw_x != draw_x_end; draw_x++, x++) {
		color4 color = gpu->nds->gpu3d.color_buf[0][gpu->nds->vcount]
		                                        [x];
		if (color.a != 0) {
			draw_3d_bg_pixel(gpu, draw_x, color, priority,
					effect_top, effect_bottom);
		}
	}
}

static u64 fetch_char_row(gpu_2d_engine *gpu, u16 se, u32 char_base, u32 bg_y,
		bool color_256);

void
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

	u32 mosaic_h = (gpu->mosaic & 0xF) + 1;
	bool apply_mosaic_h = gpu->bg_cnt[bg] & BIT(6) && mosaic_h != 1;

	u32 bg_x = gpu->bg_hofs[bg] & (bg_w - 1);
	u32 bg_y;
	if (gpu->bg_cnt[bg] & BIT(6)) {
		u32 line = gpu->nds->vcount - gpu->mosaic_countup;
		bg_y = (gpu->bg_vofs[bg] + line) & (bg_h - 1);
	} else {
		bg_y = (gpu->bg_vofs[bg] + gpu->nds->vcount) & (bg_h - 1);
	}

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

	bool effect_top = gpu->bldcnt & BIT(0 + bg);
	bool effect_bottom = gpu->bldcnt & BIT(8 + bg);

	u16 last_color = 0;
	u32 last_offset = 0;

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

		for (; color_256 && px < 8 && i < 256;
				px++, i++, char_row >>= 8) {
			bool in_mosaic = apply_mosaic_h && i % mosaic_h != 0;
			if (in_mosaic && last_offset != 0) {
				draw_2d_bg_pixel(gpu, bg, i, last_color,
						bg_priority, effect_top,
						effect_bottom);
				continue;
			}

			if (in_mosaic)
				continue;

			u32 offset = char_row & 0xFF;
			last_offset = offset;
			if (offset == 0)
				continue;

			u16 color;
			if (extended_palettes) {
				color = bg_get_color_extended(gpu, slot,
						palette_num, offset);
			} else {
				color = bg_get_color_256(gpu, offset);
			}
			last_color = color;
			draw_2d_bg_pixel(gpu, bg, i, color, bg_priority,
					effect_top, effect_bottom);
		}

		for (; !color_256 && px < 8 && i < 256;
				px++, i++, char_row >>= 4) {
			bool in_mosaic = apply_mosaic_h && i % mosaic_h != 0;
			if (in_mosaic && last_offset != 0) {
				draw_2d_bg_pixel(gpu, bg, i, last_color,
						bg_priority, effect_top,
						effect_bottom);
				continue;
			}

			if (in_mosaic)
				continue;

			u32 offset = char_row & 0xF;
			last_offset = offset;
			if (offset == 0)
				continue;

			u16 color = bg_get_color_16(gpu, palette_num, offset);
			last_color = color;
			draw_2d_bg_pixel(gpu, bg, i, color, bg_priority,
					effect_top, effect_bottom);
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

void
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
	bool effect_top = gpu->bldcnt & BIT(0 + bg);
	bool effect_bottom = gpu->bldcnt & BIT(8 + bg);

	s32 mosaic_h = (gpu->mosaic & 0xF) + 1;
	bool mosaic = gpu->bg_cnt[bg] & BIT(6) && mosaic_h != 1;

	auto step = [&](int i) {
		if (!mosaic) {
			ref_x += pa;
			ref_y += pc;
		} else if (i % mosaic_h == 0) {
			ref_x += mosaic_h * pa;
			ref_y += mosaic_h * pc;
		}
	};

	for (int i = 0; i < 256; i++, step(i)) {
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
			draw_2d_bg_pixel(gpu, bg, i, color, bg_priority,
					effect_top, effect_bottom);
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
	bool effect_top = gpu->bldcnt & BIT(0 + bg);
	bool effect_bottom = gpu->bldcnt & BIT(8 + bg);

	bool extended_palettes = gpu->dispcnt & BIT(30);
	u32 slot = bg;
	if (gpu->bg_cnt[bg] & BIT(13)) {
		slot |= 2;
	}

	s32 mosaic_h = (gpu->mosaic & 0xF) + 1;
	bool mosaic = gpu->bg_cnt[bg] & BIT(6) && mosaic_h != 1;

	auto step = [&](int i) {
		if (!mosaic) {
			ref_x += pa;
			ref_y += pc;
		} else if (i % mosaic_h == 0) {
			ref_x += mosaic_h * pa;
			ref_y += mosaic_h * pc;
		}
	};

	for (int i = 0; i < 256; i++, step(i)) {
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
			draw_2d_bg_pixel(gpu, bg, i, color, bg_priority,
					effect_top, effect_bottom);
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
	bool effect_top = gpu->bldcnt & BIT(0 + bg);
	bool effect_bottom = gpu->bldcnt & BIT(8 + bg);
	s32 mosaic_h = (gpu->mosaic & 0xF) + 1;
	bool mosaic = gpu->bg_cnt[bg] & BIT(6) && mosaic_h != 1;

	auto step = [&](int i) {
		if (!mosaic) {
			ref_x += pa;
			ref_y += pc;
		} else if (i % mosaic_h == 0) {
			ref_x += mosaic_h * pa;
			ref_y += mosaic_h * pc;
		}
	};

	for (int i = 0; i < 256; i++, step(i)) {
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
				draw_2d_bg_pixel(gpu, bg, i, color,
						bg_priority, effect_top,
						effect_bottom);
			}
		} else {
			u8 color_num = read_bg_data<u8>(
					gpu, screen_base, bg_w, bg_x, bg_y);
			if (color_num != 0) {
				u16 color = bg_get_color_256(gpu, color_num);
				draw_2d_bg_pixel(gpu, bg, i, color,
						bg_priority, effect_top,
						effect_bottom);
			}
		}
	}
}

void
render_extended_bg(gpu_2d_engine *gpu, int bg)
{
	if (!(gpu->bg_cnt[bg] & BIT(7))) {
		render_extended_text_bg(gpu, bg);
	} else {
		bool direct_color = gpu->bg_cnt[bg] & BIT(2);
		render_extended_bitmap_bg(gpu, bg, direct_color);
	}
}

void
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
	bool effect_top = gpu->bldcnt & BIT(0 + bg);
	bool effect_bottom = gpu->bldcnt & BIT(8 + bg);
	s32 mosaic_h = (gpu->mosaic & 0xF) + 1;
	bool mosaic = gpu->bg_cnt[bg] & BIT(6) && mosaic_h != 1;

	auto step = [&](int i) {
		if (!mosaic) {
			ref_x += pa;
			ref_y += pc;
		} else if (i % mosaic_h == 0) {
			ref_x += mosaic_h * pa;
			ref_y += mosaic_h * pc;
		}
	};

	for (int i = 0; i < 256; i++, step(i)) {
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
			draw_2d_bg_pixel(gpu, bg, i, color, bg_priority,
					effect_top, effect_bottom);
		}
	}
}

static void
draw_bg_pixel(gpu_2d_engine *gpu, int bg, u32 fb_x, color_u color, u8 priority,
		bool effect_top, bool effect_bottom, bool from_3d)
{
	auto& top = gpu->buffer_top[fb_x];
	auto& bottom = gpu->buffer_bottom[fb_x];

	if (gpu->window_any_enabled) {
		u32 w = gpu->window_buffer[fb_x].window;
		u8 bits = gpu->window[w].enable_bits;
		if (!(bits & BIT(bg))) {
			return;
		}
		bool effect = bits & BIT(5);
		effect_top &= effect;
		effect_bottom &= effect;
	}

	bool force_blend = from_3d;

	if (priority < top.priority) {
		bottom = top;
		top.color = color;
		top.priority = priority;
		top.effect_top = effect_top;
		top.effect_bottom = effect_bottom;
		top.force_blend = force_blend;
		top.alpha_oam = 0;
		top.from_3d = from_3d;
	} else if (priority < bottom.priority) {
		bottom.color = color;
		bottom.priority = priority;
		bottom.effect_top = effect_top;
		bottom.effect_bottom = effect_bottom;
		bottom.force_blend = force_blend;
		bottom.alpha_oam = 0;
		bottom.from_3d = from_3d;
	}
}

static void
draw_2d_bg_pixel(gpu_2d_engine *gpu, int bg, u32 fb_x, u16 color, u8 priority,
		bool effect_top, bool effect_bottom)
{
	draw_bg_pixel(gpu, bg, fb_x, color, priority, effect_top,
			effect_bottom, false);
}

static void
draw_3d_bg_pixel(gpu_2d_engine *gpu, u32 fb_x, color4 color, u8 priority,
		bool effect_top, bool effect_bottom)
{
	draw_bg_pixel(gpu, 0, fb_x, color, priority, effect_top, effect_bottom,
			true);
}

u16
bg_get_color_256(gpu_2d_engine *gpu, u32 color_num)
{
	u32 offset = 2 * color_num;
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
bg_get_color_16(gpu_2d_engine *gpu, u32 palette_num, u32 color_num)
{
	u32 offset = 32 * palette_num + 2 * color_num;
	if (gpu->engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(gpu->nds->palette, offset);
}

} // namespace twice
