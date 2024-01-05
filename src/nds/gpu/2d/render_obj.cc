#include "common/bit/byteswap.h"
#include "common/macros.h"
#include "nds/mem/vram.h"
#include "nds/nds.h"

namespace twice {

struct sprite {
	u32 id;
	u32 attrs[3];
	u32 mode;
	u32 attr;
	u32 w;
	u32 h;
	u32 char_idx;
	u32 y;
	u32 x;
	u32 x_start;
	u32 x_end;
	u32 palette_num;
	u32 tile_offset;
	u32 ref_x;
	u32 ref_y;
	s32 pa;
	u32 pc;
	bool map_1d;
	bool hflip;
};

static const u32 obj_widths[4][4] = {
	{ 8, 16, 32, 64 },
	{ 16, 32, 32, 64 },
	{ 8, 8, 16, 32 },
	{ 0, 0, 0, 0 },
};

static const u32 obj_heights[4][4] = {
	{ 8, 16, 32, 64 },
	{ 8, 8, 16, 32 },
	{ 16, 32, 32, 64 },
	{ 0, 0, 0, 0 },
};

/* setup functions */
FORCE_INLINE static void setup_sprite(sprite&);
FORCE_INLINE static int setup_sprite_bounds(
		gpu_2d_engine *, sprite&, u32 y, u32 box_w, u32 box_h);
FORCE_INLINE static int setup_non_affine_sprite(
		gpu_2d_engine *, sprite&, u32 y);
FORCE_INLINE static int setup_affine_sprite(gpu_2d_engine *, sprite&, u32 y);

/* non-affine sprites */
FORCE_INLINE static void render_normal_sprite(gpu_2d_engine *, sprite&, u32 y);
template <bool ext>
FORCE_INLINE static void render_normal_sprite_256(
		gpu_2d_engine *, sprite&, u32 px);
FORCE_INLINE static void render_normal_sprite_16(
		gpu_2d_engine *, sprite&, u32 px);
template <typename T>
FORCE_INLINE static T fetch_obj_tile(gpu_2d_engine *, sprite&);
FORCE_INLINE static void render_bitmap_sprite(gpu_2d_engine *, sprite&, u32 y);

/* affine sprites */
FORCE_INLINE static void render_affine_sprite(gpu_2d_engine *, sprite&, u32 y);
template <bool ext>
FORCE_INLINE static void render_affine_sprite_256_inner(
		gpu_2d_engine *, sprite&, u32 x);
FORCE_INLINE static void render_affine_sprite_16_inner(
		gpu_2d_engine *, sprite&, u32 x);
FORCE_INLINE static void render_affine_bitmap_sprite(
		gpu_2d_engine *, sprite&, u32 y);
FORCE_INLINE static bool affine_check_in_bounds(sprite& obj);
FORCE_INLINE static void affine_loop_step(sprite& obj);

/* helper functions */
template <typename T>
FORCE_INLINE static T read_obj_data(gpu_2d_engine *, u32 offset);
FORCE_INLINE static u16 obj_get_color_256_ext(
		gpu_2d_engine *, u32 palette_num, u32 color_num);
FORCE_INLINE static u16 obj_get_color(gpu_2d_engine *, u32 color_num);
FORCE_INLINE static u16 obj_get_color_16(
		gpu_2d_engine *, u32 palette_num, u32 color_num);
FORCE_INLINE static void write_obj_window(gpu_2d_engine *gpu, u32 x);
FORCE_INLINE static void write_obj_pixel(
		gpu_2d_engine *, u32 x, u16 color, u32 attr);

void
render_obj_line(gpu_2d_engine *gpu, u32 y)
{
	u32 oam_offset = gpu->palette_offset;

	for (u32 id = 0; id < 128; id++, oam_offset += 8) {
		sprite obj;
		obj.id = id;
		obj.attrs[0] = readarr<u16>(gpu->nds->oam, oam_offset);
		obj.attrs[1] = readarr<u16>(gpu->nds->oam, oam_offset + 2);
		obj.attrs[2] = readarr<u16>(gpu->nds->oam, oam_offset + 4);
		obj.mode = obj.attrs[0] >> 10 & 3;

		if ((obj.attrs[0] >> 8 & 3) == 2)
			continue;

		bool affine = obj.attrs[0] & BIT(8);
		bool bitmap = obj.mode == 3;
		obj.map_1d = gpu->dispcnt & BIT(bitmap ? 6 : 4);

		if (affine && bitmap) {
			render_affine_bitmap_sprite(gpu, obj, y);
		} else if (affine) {
			render_affine_sprite(gpu, obj, y);
		} else if (bitmap) {
			render_bitmap_sprite(gpu, obj, y);
		} else {
			render_normal_sprite(gpu, obj, y);
		}
	}
}

static void
setup_sprite(sprite& obj)
{
	u32 shape_bits = obj.attrs[0] >> 14 & 3;
	u32 size_bits = obj.attrs[1] >> 14 & 3;
	obj.w = obj_widths[shape_bits][size_bits];
	obj.h = obj_heights[shape_bits][size_bits];
	obj.char_idx = obj.attrs[2] & 0x3FF;
}

static int
setup_sprite_bounds(gpu_2d_engine *, sprite& obj, u32 y, u32 box_w, u32 box_h)
{
	u32 ycoord = obj.attrs[0] & 0xFF;
	obj.y = (y - ycoord) & 0xFF;
	if (obj.y >= box_h)
		return 1;

	u32 xcoord = obj.attrs[1] & 0x1FF;
	if (xcoord < 256) {
		obj.x = 0;
		obj.x_start = xcoord;
		obj.x_end = std::min<u32>(xcoord + box_w, 256);
	} else {
		obj.x = 512 - xcoord;
		if (obj.x >= box_w)
			return 1;
		obj.x_start = 0;
		obj.x_end = box_w - obj.x;
	}

	return 0;
}

static int
setup_non_affine_sprite(gpu_2d_engine *gpu, sprite& obj, u32 y)
{
	setup_sprite(obj);

	if (setup_sprite_bounds(gpu, obj, y, obj.w, obj.h))
		return 1;

	if (obj.attrs[1] & BIT(13))
		obj.y = obj.h - 1 - obj.y;

	obj.hflip = obj.attrs[1] & BIT(12);
	if (obj.hflip) {
		obj.x = obj.w - 1 - obj.x;
	}

	return 0;
}

static int
setup_affine_sprite(gpu_2d_engine *gpu, sprite& obj, u32 y)
{
	setup_sprite(obj);

	u32 box_w = obj.w;
	u32 box_h = obj.h;
	if (obj.attrs[0] & BIT(9)) {
		box_w <<= 1;
		box_h <<= 1;
	}

	if (setup_sprite_bounds(gpu, obj, y, box_w, box_h))
		return 1;

	u32 affine_idx = obj.attrs[1] >> 9 & 0x1F;
	u32 affine_addr = gpu->palette_offset + (affine_idx << 5) + 6;
	s32 pa = (s16)readarr<u16>(gpu->nds->oam, affine_addr);
	s32 pb = (s16)readarr<u16>(gpu->nds->oam, affine_addr + 8);
	s32 pc = (s16)readarr<u16>(gpu->nds->oam, affine_addr + 16);
	s32 pd = (s16)readarr<u16>(gpu->nds->oam, affine_addr + 24);
	obj.ref_x = (s32)(obj.y - (box_h >> 1)) * pb +
	            (s32)(obj.x - (box_w >> 1)) * pa + (obj.w >> 1 << 8);
	obj.ref_y = (s32)(obj.y - (box_h >> 1)) * pd +
	            (s32)(obj.x - (box_w >> 1)) * pc + (obj.h >> 1 << 8);
	obj.pa = pa;
	obj.pc = pc;

	return 0;
}

static void
render_normal_sprite(gpu_2d_engine *gpu, sprite& obj, u32 y)
{
	if (setup_non_affine_sprite(gpu, obj, y))
		return;

	bool color_256 = obj.attrs[0] & BIT(13);
	bool ext_palettes = gpu->dispcnt & BIT(31);

	/* palette_num */
	obj.palette_num = obj.attrs[2] >> 12;

	/* tile offset */
	u32 tx = obj.x >> 3;
	u32 ty = obj.y >> 3;
	if (obj.map_1d) {
		obj.tile_offset = obj.char_idx << 5
		                               << (gpu->dispcnt >> 20 & 3);
		if (color_256) {
			obj.tile_offset += (obj.w >> 3 << 6) * ty + (tx << 6);
		} else {
			obj.tile_offset += (obj.w >> 3 << 5) * ty + (tx << 5);
		}
	} else {
		if (color_256) {
			obj.tile_offset = ((obj.char_idx & ~1) << 5) +
			                  (ty << 10) + (tx << 6);
		} else {
			obj.tile_offset = (obj.char_idx << 5) + (ty << 10) +
			                  (tx << 5);
		}
	}

	obj.attr = (u32)(obj.attrs[2] >> 10 & 3) << 28 | obj.id << 16 |
	           (obj.mode == 1 ? BIT(1) : 0);
	u32 px = obj.hflip ? 7 - (obj.x & 7) : obj.x & 7;

	if (color_256 && ext_palettes)
		render_normal_sprite_256<true>(gpu, obj, px);
	else if (color_256)
		render_normal_sprite_256<false>(gpu, obj, px);
	else
		render_normal_sprite_16(gpu, obj, px);
}

template <bool ext>
static void
render_normal_sprite_256(gpu_2d_engine *gpu, sprite& obj, u32 px)
{
	for (u32 x = obj.x_start, end = obj.x_end; x != end;) {
		u64 char_row = fetch_obj_tile<u64>(gpu, obj);
		char_row >>= px << 3;

		for (; px < 8 && x != end; px++, x++, char_row >>= 8) {
			u32 color_num = char_row & 0xFF;
			if (color_num == 0)
				continue;

			u16 color;
			if (ext) {
				color = obj_get_color_256_ext(gpu,
						obj.palette_num, color_num);
			} else {
				color = obj_get_color(gpu, color_num);
			}

			if (obj.mode == 2) {
				write_obj_window(gpu, x);
			} else {
				write_obj_pixel(gpu, x, color, obj.attr);
			}
		}

		px = 0;
		obj.tile_offset += obj.hflip ? -64 : 64;
	}
}

static void
render_normal_sprite_16(gpu_2d_engine *gpu, sprite& obj, u32 px)
{
	for (u32 x = obj.x_start, end = obj.x_end; x != end;) {
		u32 char_row = fetch_obj_tile<u32>(gpu, obj);
		char_row >>= px << 2;

		for (; px < 8 && x != end; px++, x++, char_row >>= 4) {
			u32 color_num = char_row & 0xF;
			if (color_num == 0)
				continue;

			u16 color = obj_get_color_16(
					gpu, obj.palette_num, color_num);

			if (obj.mode == 2) {
				write_obj_window(gpu, x);
			} else {
				write_obj_pixel(gpu, x, color, obj.attr);
			}
		}

		px = 0;
		obj.tile_offset += obj.hflip ? -32 : 32;
	}
}

template <typename T>
static T
fetch_obj_tile(gpu_2d_engine *gpu, sprite& obj)
{
	u32 py = obj.y & 7;

	if constexpr (sizeof(T) == 8) {
		u64 char_row = read_obj_data<u64>(
				gpu, obj.tile_offset + (py << 3));
		if (obj.hflip) {
			char_row = byteswap64(char_row);
		}

		return char_row;
	} else {
		u32 char_row = read_obj_data<u32>(
				gpu, obj.tile_offset + (py << 2));
		if (obj.hflip) {
			char_row = byteswap32(char_row);
			char_row = (char_row & 0x0F0F0F0F) << 4 |
			           (char_row & 0xF0F0F0F0) >> 4;
		}

		return char_row;
	}
}

static void
render_bitmap_sprite(gpu_2d_engine *gpu, sprite& obj, u32 y)
{
	if (obj.attrs[2] >> 12 == 0)
		return;

	if (setup_non_affine_sprite(gpu, obj, y))
		return;

	u32 offset;
	if (obj.map_1d) {
		offset = obj.char_idx << 7 << (gpu->dispcnt >> 22 & 1);
		offset += (obj.w << 1) * obj.y + (obj.x << 1);
	} else {
		u32 mask = gpu->dispcnt & BIT(5) ? 0x1F : 0xF;
		offset = ((obj.char_idx & mask) << 4) +
		         ((obj.char_idx & ~mask) << 7);
		offset += (256 << (gpu->dispcnt >> 5 & 1)) * obj.y +
		          (obj.x << 1);
	}

	obj.attr = (u32)(obj.attrs[2] >> 10 & 3) << 28 | obj.id << 16 |
	           BIT(1) | (obj.attrs[2] & 0xF000);

	for (u32 x = obj.x_start, end = obj.x_end; x < end; x++) {
		u16 color = read_obj_data<u16>(gpu, offset);
		if (color & BIT(15)) {
			write_obj_pixel(gpu, x, color, obj.attr);
		}

		offset += obj.hflip ? -2 : 2;
	}
}

static void
render_affine_sprite(gpu_2d_engine *gpu, sprite& obj, u32 y)
{
	if (setup_affine_sprite(gpu, obj, y))
		return;

	bool color_256 = obj.attrs[0] & BIT(13);
	bool ext_palettes = gpu->dispcnt & BIT(31);
	obj.palette_num = obj.attrs[2] >> 12;
	obj.attr = (u32)(obj.attrs[2] >> 10 & 3) << 28 | obj.id << 16 |
	           (obj.mode == 1 ? BIT(1) : 0);

	if (color_256 && ext_palettes) {
		for (u32 x = obj.x_start, end = obj.x_end; x != end;
				x++, affine_loop_step(obj)) {
			render_affine_sprite_256_inner<true>(gpu, obj, x);
		}
	} else if (color_256) {
		for (u32 x = obj.x_start, end = obj.x_end; x != end;
				x++, affine_loop_step(obj)) {
			render_affine_sprite_256_inner<false>(gpu, obj, x);
		}
	} else {
		for (u32 x = obj.x_start, end = obj.x_end; x != end;
				x++, affine_loop_step(obj)) {
			render_affine_sprite_16_inner(gpu, obj, x);
		}
	}
}

template <bool ext>
static void
render_affine_sprite_256_inner(gpu_2d_engine *gpu, sprite& obj, u32 x)
{
	if (!affine_check_in_bounds(obj))
		return;

	u32 tx = obj.x >> 3, ty = obj.y >> 3;
	u32 px = obj.x & 7, py = obj.y & 7;

	if (obj.map_1d) {
		obj.tile_offset = obj.char_idx << 5
		                               << (gpu->dispcnt >> 20 & 3);
		obj.tile_offset += (obj.w >> 3 << 6) * ty + (tx << 6) +
		                   (py << 3) + px;
	} else {
		obj.tile_offset = ((obj.char_idx & ~1) << 5) + (ty << 10) +
		                  (tx << 6) + (py << 3) + px;
	}

	u32 color_num = read_obj_data<u8>(gpu, obj.tile_offset);
	if (color_num == 0)
		return;

	u16 color;
	if (ext) {
		color = obj_get_color_256_ext(gpu, obj.palette_num, color_num);
	} else {
		color = obj_get_color(gpu, color_num);
	}

	if (obj.mode == 2) {
		write_obj_window(gpu, x);
	} else {
		write_obj_pixel(gpu, x, color, obj.attr);
	}
}

static void
render_affine_sprite_16_inner(gpu_2d_engine *gpu, sprite& obj, u32 x)
{
	if (!affine_check_in_bounds(obj))
		return;

	u32 tx = obj.x >> 3, ty = obj.y >> 3;
	u32 px = obj.x & 7, py = obj.y & 7;

	if (obj.map_1d) {
		obj.tile_offset = obj.char_idx << 5
		                               << (gpu->dispcnt >> 20 & 3);
		obj.tile_offset += (obj.w >> 3 << 5) * ty + (tx << 5) +
		                   (py << 2) + (px >> 1);
	} else {
		obj.tile_offset = (obj.char_idx << 5) + (ty << 10) +
		                  (tx << 5) + (py << 2) + (px >> 1);
	}

	u32 color_num = read_obj_data<u8>(gpu, obj.tile_offset);
	color_num = px & 1 ? color_num >> 4 : color_num & 0xF;
	if (color_num == 0)
		return;

	u16 color = obj_get_color_16(gpu, obj.palette_num, color_num);

	if (obj.mode == 2) {
		write_obj_window(gpu, x);
	} else {
		write_obj_pixel(gpu, x, color, obj.attr);
	}
}

static void
render_affine_bitmap_sprite(gpu_2d_engine *gpu, sprite& obj, u32 y)
{
	if (obj.attrs[2] >> 12 == 0)
		return;

	if (setup_affine_sprite(gpu, obj, y))
		return;

	u32 base_offset;
	if (obj.map_1d) {
		base_offset = obj.char_idx << 7 << (gpu->dispcnt >> 22 & 1);
	} else {
		u32 mask = gpu->dispcnt & BIT(5) ? 0x1F : 0xF;
		base_offset = ((obj.char_idx & mask) << 4) +
		              ((obj.char_idx & ~mask) << 7);
	}
	u32 width_2d_shift = 8 + (gpu->dispcnt >> 5 & 1);

	obj.attr = (u32)(obj.attrs[2] >> 10 & 3) << 28 | obj.id << 16 |
	           BIT(1) | (obj.attrs[2] & 0xF000);

	for (u32 x = obj.x_start, end = obj.x_end; x != end;
			x++, affine_loop_step(obj)) {
		if (!affine_check_in_bounds(obj))
			continue;

		u32 offset = base_offset;
		if (obj.map_1d) {
			offset += (obj.w << 1) * obj.y + (obj.x << 1);
		} else {
			offset += (obj.y << width_2d_shift) + (obj.x << 1);
		}

		u16 color = read_obj_data<u16>(gpu, offset);
		if (color & BIT(15)) {
			write_obj_pixel(gpu, x, color, obj.attr);
		}
	}
}

static bool
affine_check_in_bounds(sprite& obj)
{
	obj.x = obj.ref_x >> 8;
	obj.y = obj.ref_y >> 8;

	return !(((s32)obj.x < 0 || obj.x >= obj.w || (s32)obj.y < 0 ||
			obj.y >= obj.h));
}

static void
affine_loop_step(sprite& obj)
{
	obj.ref_x += obj.pa;
	obj.ref_y += obj.pc;
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

static u16
obj_get_color_256_ext(gpu_2d_engine *gpu, u32 palette_num, u32 color_num)
{
	u32 offset = 512 * palette_num + 2 * color_num;
	if (gpu->engineid == 0) {
		return vram_read_aobj_palette<u16>(gpu->nds, offset);
	} else {
		return vram_read_bobj_palette<u16>(gpu->nds, offset);
	}
}

static u16
obj_get_color(gpu_2d_engine *gpu, u32 color_num)
{
	u32 offset = 0x200 + 2 * color_num;
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

static void
write_obj_window(gpu_2d_engine *gpu, u32 x)
{
	if (gpu->window_enabled[2]) {
		gpu->active_window[x] = 2;
	}
}

static void
write_obj_pixel(gpu_2d_engine *gpu, u32 x, u16 color, u32 attr)
{
	if (attr < gpu->obj_attr[x]) {
		gpu->obj_line[x] = color;
		gpu->obj_attr[x] = attr;
	}
}

} // namespace twice
