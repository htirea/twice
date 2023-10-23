#include "nds/mem/vram.h"
#include "nds/nds.h"

namespace twice {

struct obj_data {
	u16 attr0;
	u16 attr1;
	u16 attr2;
	u32 w;
	u32 h;
	u8 mode;
};

static void read_oam_attrs(gpu_2d_engine *gpu, int obj_num, obj_data *obj);
static void render_normal_sprite(gpu_2d_engine *gpu, obj_data *obj);
static void render_bitmap_sprite(gpu_2d_engine *gpu, obj_data *obj);
static void render_affine_sprite(gpu_2d_engine *gpu, obj_data *obj);
static void render_affine_bitmap_sprite(gpu_2d_engine *gpu, obj_data *obj);

void
render_sprites(gpu_2d_engine *gpu)
{
	for (int i = 0; i < 128; i++) {
		obj_data obj;
		read_oam_attrs(gpu, i, &obj);

		if ((obj.attr0 >> 8 & 3) == 2) {
			continue;
		}

		bool is_affine = obj.attr0 & BIT(8);
		bool is_bitmap = obj.mode == 3;

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
	obj->mode = obj->attr0 >> 10 & 3;
}

static void get_obj_size(obj_data *obj, u32 *obj_w, u32 *obj_h);
static void draw_obj_pixel(gpu_2d_engine *gpu, u32 fb_x, color_u color,
		u8 priority, bool force_blend, u8 alpha_oam);
static u16 obj_get_color_256(gpu_2d_engine *gpu, u32 color_num);
static u16 obj_get_color_extended(
		gpu_2d_engine *gpu, u32 palette_num, u32 color_num);
static u16 obj_get_color_16(
		gpu_2d_engine *gpu, u32 palette_num, u32 color_num);

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

static u64 fetch_obj_char_row(gpu_2d_engine *gpu, u32 tile_offset, u32 py,
		bool hflip, bool color_256);

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
	bool force_blend = obj->mode == 1;
	bool obj_window = obj->mode == 2;

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
				if (offset == 0)
					continue;

				u16 color;
				if (extended_palettes) {
					color = obj_get_color_extended(gpu,
							palette_num, offset);
				} else {
					color = obj_get_color_256(gpu, offset);
				}
				if (obj_window && gpu->window_enabled[2]) {
					gpu->window_buffer[draw_x].window = 2;
				} else if (!obj_window) {
					draw_obj_pixel(gpu, draw_x, color,
							priority, force_blend,
							0);
				}
			}
		} else {
			for (; px < 8 && draw_x != draw_x_end;
					px++, draw_x++, char_row >>= 4) {
				u32 offset = char_row & 0xF;
				if (offset == 0)
					continue;
				u16 color = obj_get_color_16(
						gpu, palette_num, offset);
				if (obj_window && gpu->window_enabled[2]) {
					gpu->window_buffer[draw_x].window = 2;
				} else if (!obj_window) {
					draw_obj_pixel(gpu, draw_x, color,
							priority, force_blend,
							0);
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
render_bitmap_sprite(gpu_2d_engine *gpu, obj_data *obj)
{
	u16 alpha_oam = obj->attr2 >> 12;
	if (alpha_oam == 0) {
		return;
	}

	get_obj_size(obj, &obj->w, &obj->h);

	u32 obj_attr_y = obj->attr0 & 0xFF;
	u32 obj_y = (gpu->nds->vcount - obj_attr_y) & 0xFF;
	if (obj_y >= obj->h) {
		return;
	}
	if (obj->attr1 & BIT(13)) {
		obj_y = obj->h - 1 - obj_y;
	}
	bool hflip = obj->attr1 & BIT(12);

	u32 obj_attr_x = obj->attr1 & 0x1FF;
	u32 obj_x, draw_x, draw_x_end;
	if (obj_attr_x < 256) {
		obj_x = 0;
		draw_x = obj_attr_x;
		draw_x_end = std::min(obj_attr_x + obj->w, (u32)256);
	} else {
		obj_x = 512 - obj_attr_x;
		if (obj_x >= obj->w) {
			return;
		}
		draw_x = 0;
		draw_x_end = obj->w - obj_x;
	}

	if (hflip) {
		obj_x = obj->w - 1 - obj_x;
	}

	u32 priority = obj->attr2 >> 10 & 3;
	u32 obj_char_name = obj->attr2 & 0x3FF;
	bool map_1d = gpu->dispcnt & BIT(6);
	u32 width_2d = 128 << (gpu->dispcnt >> 5 & 1);

	u32 offset;
	if (map_1d) {
		offset = obj_char_name << 7 << (gpu->dispcnt >> 22 & 1);
		offset += obj->w * 2 * obj_y + 2 * obj_x;
	} else {
		u32 mask_x = gpu->dispcnt & BIT(5) ? 0x1F : 0xF;
		offset = 0x10 * (obj_char_name & mask_x) +
		         0x80 * (obj_char_name & ~mask_x);
		offset += width_2d * 2 * obj_y + 2 * obj_x;
	}

	for (u32 i = draw_x; i < draw_x_end; i++) {
		u16 color = read_obj_data<u16>(gpu, offset);
		if (color & BIT(15)) {
			draw_obj_pixel(gpu, i, color, priority, true,
					alpha_oam);
		}

		offset = hflip ? offset - 2 : offset + 2;
	}
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
	bool force_blend = obj->mode == 1;
	bool obj_window = obj->mode == 2;

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
				if (obj_window && gpu->window_enabled[2]) {
					gpu->window_buffer[i].window = 2;
				} else if (!obj_window) {
					draw_obj_pixel(gpu, i, color, priority,
							force_blend, 0);
				}
			}
		} else {
			offset = px & 1 ? offset >> 4 : offset & 0xF;
			if (offset != 0) {
				u16 color = obj_get_color_16(
						gpu, palette_num, offset);
				if (obj_window && gpu->window_enabled[2]) {
					gpu->window_buffer[i].window = 2;
				} else if (!obj_window) {
					draw_obj_pixel(gpu, i, color, priority,
							force_blend, 0);
				}
			}
		}
	}
}

static void
render_affine_bitmap_sprite(gpu_2d_engine *gpu, obj_data *obj)
{
	u16 alpha_oam = obj->attr2 >> 12;
	if (alpha_oam == 0) {
		return;
	}

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
			draw_obj_pixel(gpu, i, color, priority, true,
					alpha_oam);
		}
	}
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

static void
draw_obj_pixel(gpu_2d_engine *gpu, u32 fb_x, color_u color, u8 priority,
		bool force_blend, u8 alpha_oam)
{
	auto& top = gpu->obj_buffer[fb_x];

	if (priority < top.priority) {
		top.color = color;
		top.priority = priority;
		top.force_blend = force_blend;
		top.alpha_oam = alpha_oam;
		top.from_3d = 0;
	}
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
obj_get_color_16(gpu_2d_engine *gpu, u32 palette_num, u32 color_num)
{
	u32 offset = 0x200 + 32 * palette_num + 2 * color_num;
	if (gpu->engineid == 1) {
		offset += 0x400;
	}

	return readarr<u16>(gpu->nds->palette, offset);
}

} // namespace twice
