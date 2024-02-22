#include "nds/gpu/2d/render.h"

#include "common/bit/byteswap.h"
#include "common/macros.h"
#include "nds/mem/vram.h"
#include "nds/nds.h"

namespace twice {

struct background {
	u32 id;
	u32 priority;
	u32 attr;
	u32 size_bits;
	u32 slot;
	s32 mosaic_h;
	u32 screen_base;
	u32 char_base;
	u32 w;
	u32 h;
	u32 x;
	u32 y;
	u32 screen;
	u32 se_x;
	u32 se_y;
	s32 ref_x;
	s32 ref_y;
	s32 pa;
	s32 pc;
	bool mosaic;
	bool color_256;
	bool ext_palettes;
	bool wrap;
};

/* setup functions */
FORCE_INLINE static void setup_bg(
		gpu_2d_engine *gpu, int bg_id, background& bg);
FORCE_INLINE static void setup_affine_bg(
		gpu_2d_engine *gpu, int bg_id, background& bg);
FORCE_INLINE static void setup_default_bg_base_addr(
		gpu_2d_engine *gpu, background& bg);

/* text bg */
template <bool ext>
FORCE_INLINE static void render_text_bg_256(gpu_2d_engine *, background&);
FORCE_INLINE static void render_text_bg_16(gpu_2d_engine *, background&);
template <bool ext>
FORCE_INLINE static void render_text_bg_256_mosaic(
		gpu_2d_engine *, background&);
FORCE_INLINE static void render_text_bg_16_mosaic(
		gpu_2d_engine *, background&);
FORCE_INLINE static void render_text_bg_loop_step(background&);
template <typename T>
FORCE_INLINE static std::pair<T, u32> fetch_text_bg_tile(
		gpu_2d_engine *, background&);

/* affine bg */
FORCE_INLINE static void render_affine_bg_inner(
		gpu_2d_engine *, background&, u32 x);
FORCE_INLINE static void render_ext_text_bg_inner(
		gpu_2d_engine *, background&, u32 x);
FORCE_INLINE static void render_ext_bitmap_bg_inner(
		gpu_2d_engine *, background&, u32 x, bool direct_color);
FORCE_INLINE static void render_large_bitmap_bg_inner(
		gpu_2d_engine *, background&, u32 x);
FORCE_INLINE static bool affine_check_in_bounds(
		gpu_2d_engine *, background&, u32 x);
FORCE_INLINE static void affine_loop_step(background&);
FORCE_INLINE static void affine_loop_step_mosaic(background&, u32 x);

/* helper functions */
template <typename T>
FORCE_INLINE static T read_bg_data(gpu_2d_engine *, u32 offset);
FORCE_INLINE static u16 bg_get_color_256_ext(
		gpu_2d_engine *, u32 slot, u32 palette_num, u32 color_num);
FORCE_INLINE static u16 bg_get_color_16(
		gpu_2d_engine *, u32 palette_num, u32 color_num);
FORCE_INLINE static void write_pixel(
		gpu_2d_engine *, u32 bg_id, u32 x, color_u color, u32 attr);

void
render_text_bg_line(gpu_2d_engine *gpu, int bg_id, u32 y)
{
	background bg;
	setup_bg(gpu, bg_id, bg);
	setup_default_bg_base_addr(gpu, bg);

	bg.w = 256 << (bg.size_bits & 1);
	bg.h = 256 << (bg.size_bits >> 1);
	bg.x = gpu->bg_hofs[bg.id];
	bg.y = gpu->bg_vofs[bg.id] + y - (bg.mosaic ? gpu->mosaic_countup : 0);
	bg.x &= bg.w - 1;
	bg.y &= bg.h - 1;
	bg.screen = ((bg.y >> 8 << 1) + (bg.x >> 8)) >>
	            (bg.size_bits == 2 ? 1 : 0);
	bg.se_x = (bg.x & 255) >> 3;
	bg.se_y = (bg.y & 255) >> 3;

	if (!bg.mosaic_h) {
		if (bg.color_256 && bg.ext_palettes)
			render_text_bg_256<true>(gpu, bg);
		else if (bg.color_256)
			render_text_bg_256<false>(gpu, bg);
		else
			render_text_bg_16(gpu, bg);
	} else {
		if (bg.color_256 && bg.ext_palettes)
			render_text_bg_256_mosaic<true>(gpu, bg);
		else if (bg.color_256)
			render_text_bg_256_mosaic<false>(gpu, bg);
		else
			render_text_bg_16_mosaic(gpu, bg);
	}
}

void
render_affine_bg_line(gpu_2d_engine *gpu, int bg_id, u32)
{
	background bg;
	setup_affine_bg(gpu, bg_id, bg);
	setup_default_bg_base_addr(gpu, bg);

	bg.w = 128 << bg.size_bits;
	bg.h = bg.w;

	if (!bg.mosaic_h) {
		for (u32 x = 0; x < 256; x++, affine_loop_step(bg)) {
			render_affine_bg_inner(gpu, bg, x);
		}
	} else {
		for (u32 x = 0; x < 256; x++, affine_loop_step_mosaic(bg, x)) {
			render_affine_bg_inner(gpu, bg, x);
		}
	}
}

void
render_extended_bg_line(gpu_2d_engine *gpu, int bg_id, u32 y)
{
	if (!(gpu->bg_cnt[bg_id] & BIT(7))) {
		render_ext_text_bg_line(gpu, bg_id, y);
	} else {
		bool direct_color = gpu->bg_cnt[bg_id] & BIT(2);
		render_ext_bitmap_bg_line(gpu, bg_id, y, direct_color);
	}
}

void
render_ext_text_bg_line(gpu_2d_engine *gpu, int bg_id, u32)
{
	background bg;
	setup_affine_bg(gpu, bg_id, bg);
	setup_default_bg_base_addr(gpu, bg);

	bg.w = 128 << bg.size_bits;
	bg.h = bg.w;

	if (!bg.mosaic_h) {
		for (u32 x = 0; x < 256; x++, affine_loop_step(bg)) {
			render_ext_text_bg_inner(gpu, bg, x);
		}
	} else {
		for (u32 x = 0; x < 256; x++, affine_loop_step_mosaic(bg, x)) {
			render_ext_text_bg_inner(gpu, bg, x);
		}
	}
}

void
render_ext_bitmap_bg_line(
		gpu_2d_engine *gpu, int bg_id, u32, bool direct_color)
{
	background bg;
	setup_affine_bg(gpu, bg_id, bg);

	bg.screen_base = (u32)(gpu->bg_cnt[bg.id] >> 8 & 0x1F) << 14;
	u32 widths[] = { 128, 256, 512, 512 };
	u32 heights[] = { 128, 256, 256, 512 };
	bg.w = widths[bg.size_bits];
	bg.h = heights[bg.size_bits];

	if (!bg.mosaic_h) {
		for (u32 x = 0; x < 256; x++, affine_loop_step(bg)) {
			render_ext_bitmap_bg_inner(gpu, bg, x, direct_color);
		}
	} else {
		for (u32 x = 0; x < 256; x++, affine_loop_step_mosaic(bg, x)) {
			render_ext_bitmap_bg_inner(gpu, bg, x, direct_color);
		}
	}
}

void
render_large_bmp_bg_line(gpu_2d_engine *gpu, u32)
{
	background bg;
	setup_affine_bg(gpu, 2, bg);

	switch (bg.size_bits) {
	case 0:
		bg.w = 512;
		bg.h = 1024;
		break;
	case 1:
		bg.w = 1024;
		bg.h = 512;
		break;
	default:
		throw twice_error("large bitmap bg invalid size");
	}

	if (!bg.mosaic_h) {
		for (u32 x = 0; x < 256; x++, affine_loop_step(bg)) {
			render_large_bitmap_bg_inner(gpu, bg, x);
		}
	} else {
		for (u32 x = 0; x < 256; x++, affine_loop_step_mosaic(bg, x)) {
			render_large_bitmap_bg_inner(gpu, bg, x);
		}
	}
}

void
render_3d_line(gpu_2d_engine *gpu, u32 y)
{
	u32 priority = gpu->bg_cnt[0] & 3;
	u32 attr = priority << 28 | (u32)4 << 24 | (gpu->bldcnt & 0x101) | 0x6;
	s32 offset = SEXT<9>(gpu->bg_hofs[0]);

	u32 x, start, end;
	if (offset < 0) {
		x = 0;
		start = -offset;
		end = 256;
	} else {
		x = offset;
		start = 0;
		end = 256 - offset;
	}

	for (; start != end; start++, x++) {
		if (!layer_in_window(gpu, 0, x))
			continue;

		color4 color = gpu->nds->gpu3d.re.color_buf[0][y][x];
		if (color.a == 0)
			continue;

		write_pixel(gpu, 0, start, color, attr);
	}
}

static void
setup_bg(gpu_2d_engine *gpu, int bg_id, background& bg)
{
	bg.id = bg_id;
	bg.priority = gpu->bg_cnt[bg.id] & 3;
	bg.attr = bg.priority << 28 | (bg.id + 4) << 24 |
	          (gpu->bldcnt >> bg.id & 0x101);
	bg.size_bits = gpu->bg_cnt[bg.id] >> 14 & 3;
	bg.slot = bg.id | (gpu->bg_cnt[bg.id] & BIT(13) ? 2 : 0);
	bg.color_256 = gpu->bg_cnt[bg.id] & BIT(7);
	bg.ext_palettes = gpu->dispcnt & BIT(30);
	bg.mosaic = gpu->bg_cnt[bg.id] & BIT(6);
	bg.mosaic_h = bg.mosaic ? gpu->mosaic & 0xF : 0;
}

static void
setup_affine_bg(gpu_2d_engine *gpu, int bg_id, background& bg)
{
	setup_bg(gpu, bg_id, bg);

	bg.wrap = gpu->bg_cnt[bg.id] & BIT(13);
	bg.ref_x = gpu->bg_ref_x[bg.id - 2];
	bg.ref_y = gpu->bg_ref_y[bg.id - 2];
	bg.pa = (s16)gpu->bg_pa[bg.id - 2];
	bg.pc = (s16)gpu->bg_pc[bg.id - 2];
}

static void
setup_default_bg_base_addr(gpu_2d_engine *gpu, background& bg)
{
	bg.screen_base = ((u32)(gpu->bg_cnt[bg.id] >> 8 & 0x1F) << 11) +
	                 ((gpu->dispcnt >> 27 & 7) << 16);
	bg.char_base = ((u32)(gpu->bg_cnt[bg.id] >> 2 & 0xF) << 14) +
	               ((gpu->dispcnt >> 24 & 7) << 16);
}

template <bool ext>
static void
render_text_bg_256(gpu_2d_engine *gpu, background& bg)
{
	for (u32 x = 0, px = bg.x & 7; x < 256;) {
		auto [char_row, palette_num] =
				fetch_text_bg_tile<u64>(gpu, bg);
		char_row >>= px << 3;

		for (; px < 8 && x < 256; px++, x++, char_row >>= 8) {
			if (!layer_in_window(gpu, bg.id, x))
				continue;

			u32 color_num = char_row & 0xFF;
			if (color_num == 0)
				continue;

			u16 color;
			if (ext) {
				color = bg_get_color_256_ext(gpu, bg.slot,
						palette_num, color_num);
			} else {
				color = bg_get_color(gpu, color_num);
			}

			write_pixel(gpu, bg.id, x, color, bg.attr);
		}

		render_text_bg_loop_step(bg);
		px = 0;
	}
}

static void
render_text_bg_16(gpu_2d_engine *gpu, background& bg)
{
	for (u32 x = 0, px = bg.x & 7; x < 256;) {
		auto [char_row, palette_num] =
				fetch_text_bg_tile<u32>(gpu, bg);
		char_row >>= px << 2;

		for (; px < 8 && x < 256; px++, x++, char_row >>= 4) {
			if (!layer_in_window(gpu, bg.id, x))
				continue;

			u32 color_num = char_row & 0xF;
			if (color_num == 0)
				continue;

			u16 color = bg_get_color_16(
					gpu, palette_num, color_num);
			write_pixel(gpu, bg.id, x, color, bg.attr);
		}

		render_text_bg_loop_step(bg);
		px = 0;
	}
}

template <bool ext>
static void
render_text_bg_256_mosaic(gpu_2d_engine *gpu, background& bg)
{
	u32 mosaic_w = bg.mosaic_h + 1;
	auto [char_row, palette_num] = fetch_text_bg_tile<u64>(gpu, bg);

	for (u32 x = 0, px = 0, k = bg.x & 7; x < 256;) {
		u32 num_tiles = ((px + k) >> 3) - (px >> 3);
		px = (px + k) & 7;

		for (u32 i = num_tiles; i--;) {
			render_text_bg_loop_step(bg);
		}

		if (num_tiles != 0) {
			std::tie(char_row, palette_num) =
					fetch_text_bg_tile<u64>(gpu, bg);
			char_row >>= px << 3;
		} else {
			char_row >>= k << 3;
		}

		u32 color_num = char_row & 0xFF;
		if (color_num == 0) {
			x += mosaic_w;
			k = mosaic_w;
			continue;
		}

		u16 color;
		if (ext) {
			color = bg_get_color_256_ext(
					gpu, bg.slot, palette_num, color_num);
		} else {
			color = bg_get_color(gpu, color_num);
		}

		for (k = 0; k < mosaic_w && x < 256; k++, x++) {
			if (!layer_in_window(gpu, bg.id, x))
				continue;

			write_pixel(gpu, bg.id, x, color, bg.attr);
		}
	}
}

static void
render_text_bg_16_mosaic(gpu_2d_engine *gpu, background& bg)
{
	u32 mosaic_w = bg.mosaic_h + 1;
	auto [char_row, palette_num] = fetch_text_bg_tile<u32>(gpu, bg);

	for (u32 x = 0, px = 0, k = bg.x & 7; x < 256;) {
		u32 num_tiles = ((px + k) >> 3) - (px >> 3);
		px = (px + k) & 7;

		for (u32 i = num_tiles; i--;) {
			render_text_bg_loop_step(bg);
		}

		if (num_tiles != 0) {
			std::tie(char_row, palette_num) =
					fetch_text_bg_tile<u32>(gpu, bg);
			char_row >>= px << 2;
		} else {
			char_row >>= k << 2;
		}

		u32 color_num = char_row & 0xF;
		if (color_num == 0) {
			x += mosaic_w;
			k = mosaic_w;
			continue;
		}

		u16 color = bg_get_color_16(gpu, palette_num, color_num);

		for (k = 0; k < mosaic_w && x < 256; k++, x++) {
			if (!layer_in_window(gpu, bg.id, x))
				continue;

			write_pixel(gpu, bg.id, x, color, bg.attr);
		}
	}
}

static void
render_text_bg_loop_step(background& bg)
{
	bg.se_x += 1;
	if (bg.se_x >= 32) {
		bg.se_x = 0;
		if (bg.w > 256) {
			bg.screen ^= 1;
		}
	}
}

template <typename T>
static std::pair<T, u32>
fetch_text_bg_tile(gpu_2d_engine *gpu, background& bg)
{
	u32 se_offset = bg.screen_base + (bg.screen << 11) + (bg.se_y << 6) +
	                (bg.se_x << 1);
	u32 se = read_bg_data<u16>(gpu, se_offset);
	u32 py = se & BIT(11) ? 7 - (bg.y & 7) : bg.y & 7;
	u32 char_idx = se & 0x3FF;

	if constexpr (sizeof(T) == 8) {
		u64 char_row = read_bg_data<u64>(gpu,
				bg.char_base + (char_idx << 6) + (py << 3));
		if (se & BIT(10)) {
			char_row = byteswap64(char_row);
		}

		return { char_row, se >> 12 };
	} else {

		u32 char_row = read_bg_data<u32>(gpu,
				bg.char_base + (char_idx << 5) + (py << 2));
		if (se & BIT(10)) {
			char_row = byteswap32(char_row);
			char_row = (char_row & 0x0F0F0F0F) << 4 |
			           (char_row & 0xF0F0F0F0) >> 4;
		}

		return { char_row, se >> 12 };
	}
}

static void
render_affine_bg_inner(gpu_2d_engine *gpu, background& bg, u32 x)
{
	if (!affine_check_in_bounds(gpu, bg, x))
		return;

	u32 se_x = bg.x >> 3;
	u32 se_y = bg.y >> 3;
	u32 px = bg.x & 7;
	u32 py = bg.y & 7;
	u32 se_offset = bg.screen_base + (bg.w >> 3) * se_y + se_x;
	u32 se = read_bg_data<u8>(gpu, se_offset);
	u8 color_num = read_bg_data<u8>(
			gpu, bg.char_base + (se << 6) + (py << 3) + px);
	if (color_num == 0)
		return;

	u16 color = bg_get_color(gpu, color_num);
	write_pixel(gpu, bg.id, x, color, bg.attr);
}

static void
render_ext_text_bg_inner(gpu_2d_engine *gpu, background& bg, u32 x)
{
	if (!affine_check_in_bounds(gpu, bg, x))
		return;

	u32 se_x = bg.x >> 3;
	u32 se_y = bg.y >> 3;
	u32 px = bg.x & 7;
	u32 py = bg.y & 7;
	u32 se_offset = bg.screen_base + (bg.w >> 3 << 1) * se_y + (se_x << 1);
	u32 se = read_bg_data<u16>(gpu, se_offset);

	if (se & BIT(11)) {
		py = 7 - py;
	}

	if (se & BIT(10)) {
		px = 7 - px;
	}

	u32 char_idx = se & 0x3FF;
	u8 color_num = read_bg_data<u8>(
			gpu, bg.char_base + (char_idx << 6) + (py << 3) + px);
	if (color_num == 0)
		return;

	u16 color;
	if (bg.ext_palettes) {
		u32 palette_num = se >> 12;
		color = bg_get_color_256_ext(
				gpu, bg.slot, palette_num, color_num);
	} else {
		color = bg_get_color(gpu, color_num);
	}

	write_pixel(gpu, bg.id, x, color, bg.attr);
}

static void
render_ext_bitmap_bg_inner(
		gpu_2d_engine *gpu, background& bg, u32 x, bool direct_color)
{
	if (!affine_check_in_bounds(gpu, bg, x))
		return;

	if (direct_color) {
		u32 offset = bg.screen_base + (bg.w * bg.y << 1) + (bg.x << 1);
		u16 color = read_bg_data<u16>(gpu, offset);
		if (!(color & BIT(15)))
			return;

		write_pixel(gpu, bg.id, x, color, bg.attr);
	} else {
		u32 offset = bg.screen_base + bg.w * bg.y + bg.x;
		u8 color_num = read_bg_data<u8>(gpu, offset);
		if (color_num == 0)
			return;

		u16 color = bg_get_color(gpu, color_num);
		write_pixel(gpu, bg.id, x, color, bg.attr);
	}
}

static void
render_large_bitmap_bg_inner(gpu_2d_engine *gpu, background& bg, u32 x)
{
	if (!affine_check_in_bounds(gpu, bg, x))
		return;

	u8 color_num = vram_read<u8>(gpu->nds, bg.w * bg.y + bg.x);
	if (color_num == 0)
		return;

	u16 color = bg_get_color(gpu, color_num);
	write_pixel(gpu, bg.id, x, color, bg.attr);
}

static bool
affine_check_in_bounds(gpu_2d_engine *gpu, background& bg, u32 x)
{
	if (!layer_in_window(gpu, bg.id, x))
		return false;

	bg.x = bg.ref_x >> 8;
	bg.y = bg.ref_y >> 8;

	if (bg.wrap) {
		bg.x &= bg.w - 1;
		bg.y &= bg.h - 1;
	} else if ((s32)bg.x < 0 || bg.x >= bg.w || (s32)bg.y < 0 ||
			bg.y >= bg.h) {
		return false;
	}

	return true;
}

static void
affine_loop_step(background& bg)
{
	bg.ref_x += bg.pa;
	bg.ref_y += bg.pc;
}

static void
affine_loop_step_mosaic(background& bg, u32 x)
{
	if (x % (bg.mosaic_h + 1) == 0) {
		bg.ref_x += (bg.mosaic_h + 1) * bg.pa;
		bg.ref_y += (bg.mosaic_h + 1) * bg.pc;
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

static u16
bg_get_color_256_ext(
		gpu_2d_engine *gpu, u32 slot, u32 palette_num, u32 color_num)
{
	u32 offset = (slot << 13) + (palette_num << 9) + (color_num << 1);
	if (gpu->engineid == 0) {
		return vram_read_abg_palette<u16>(gpu->nds, offset);
	} else {
		return vram_read_bbg_palette<u16>(gpu->nds, offset);
	}
}

static u16
bg_get_color_16(gpu_2d_engine *gpu, u32 palette_num, u32 color_num)
{
	u32 offset = gpu->palette_offset + (palette_num << 5) +
	             (color_num << 1);
	return readarr<u16>(gpu->nds->palette, offset);
}

static void
write_pixel(gpu_2d_engine *gpu, u32 bg_id, u32 x, color_u color, u32 attr)
{
	gpu->bg_line[bg_id][x] = color;
	gpu->bg_attr[bg_id][x] = attr;
}

} // namespace twice
