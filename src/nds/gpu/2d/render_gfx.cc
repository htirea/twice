#include "nds/gpu/2d/render.h"

#include "nds/nds.h"

namespace twice {

static void setup_windows(gpu_2d_engine *gpu);
static void clear_obj_buffers(gpu_2d_engine *gpu);
static void clear_bg_buffers(gpu_2d_engine *gpu);
static void apply_window_to_obj_line(gpu_2d_engine *gpu);
static void set_active_window(gpu_2d_engine *gpu);
static void merge_lines(gpu_2d_engine *gpu, u32 y);
static color4 alpha_blend(color4 c1, color4 c2, u8 k1, u8 k2, u8 shift);
static color4 increase_brightness(color4 c, u8 k);
static color4 decrease_brightness(color4 c, u8 k);

void
render_gfx_line(gpu_2d_engine *gpu, u32 y)
{
	setup_windows(gpu);
	clear_obj_buffers(gpu);
	clear_bg_buffers(gpu);

	if (gpu->dispcnt & BIT(12)) {
		render_obj_line(gpu, y);
	}

	set_active_window(gpu);

	if (gpu->dispcnt & BIT(12)) {
		apply_window_to_obj_line(gpu);
	}

	bool render_3d = gpu->engineid == 0 && (gpu->dispcnt & BIT(3));

	switch (gpu->dispcnt & 0x7) {
	case 0:
		if (gpu->dispcnt & BIT(8)) {
			if (render_3d)
				render_3d_line(gpu, y);
			else
				render_text_bg_line(gpu, 0, y);
		}
		if (gpu->dispcnt & BIT(9))
			render_text_bg_line(gpu, 1, y);
		if (gpu->dispcnt & BIT(10))
			render_text_bg_line(gpu, 2, y);
		if (gpu->dispcnt & BIT(11))
			render_text_bg_line(gpu, 3, y);
		break;
	case 1:
		if (gpu->dispcnt & BIT(8)) {
			if (render_3d)
				render_3d_line(gpu, y);
			else
				render_text_bg_line(gpu, 0, y);
		}
		if (gpu->dispcnt & BIT(9))
			render_text_bg_line(gpu, 1, y);
		if (gpu->dispcnt & BIT(10))
			render_text_bg_line(gpu, 2, y);
		if (gpu->dispcnt & BIT(11))
			render_affine_bg_line(gpu, 3, y);
		break;
	case 2:
		if (gpu->dispcnt & BIT(8)) {
			if (render_3d)
				render_3d_line(gpu, y);
			else
				render_text_bg_line(gpu, 0, y);
		}
		if (gpu->dispcnt & BIT(9))
			render_text_bg_line(gpu, 1, y);
		if (gpu->dispcnt & BIT(10))
			render_affine_bg_line(gpu, 2, y);
		if (gpu->dispcnt & BIT(11))
			render_affine_bg_line(gpu, 3, y);
		break;
	case 3:
		if (gpu->dispcnt & BIT(8)) {
			if (render_3d)
				render_3d_line(gpu, y);
			else
				render_text_bg_line(gpu, 0, y);
		}
		if (gpu->dispcnt & BIT(9))
			render_text_bg_line(gpu, 1, y);
		if (gpu->dispcnt & BIT(10))
			render_text_bg_line(gpu, 2, y);
		if (gpu->dispcnt & BIT(11))
			render_extended_bg_line(gpu, 3, y);
		break;
	case 4:
		if (gpu->dispcnt & BIT(8)) {
			if (render_3d)
				render_3d_line(gpu, y);
			else
				render_text_bg_line(gpu, 0, y);
		}
		if (gpu->dispcnt & BIT(9))
			render_text_bg_line(gpu, 1, y);
		if (gpu->dispcnt & BIT(10))
			render_affine_bg_line(gpu, 2, y);
		if (gpu->dispcnt & BIT(11))
			render_extended_bg_line(gpu, 3, y);
		break;
	case 5:
		if (gpu->dispcnt & BIT(8)) {
			if (render_3d)
				render_3d_line(gpu, y);
			else
				render_text_bg_line(gpu, 0, y);
		}
		if (gpu->dispcnt & BIT(9))
			render_text_bg_line(gpu, 1, y);
		if (gpu->dispcnt & BIT(10))
			render_extended_bg_line(gpu, 2, y);
		if (gpu->dispcnt & BIT(11))
			render_extended_bg_line(gpu, 3, y);
		break;
	case 6:
		if (gpu->engineid == 1) {
			throw twice_error("bg mode 6 invalid for engine B");
		}
		if (gpu->dispcnt & BIT(8))
			render_3d_line(gpu, y);
		if (gpu->dispcnt & BIT(10))
			render_large_bmp_bg_line(gpu, y);
		break;
	case 7:
		throw twice_error("bg mode 7 invalid");
	}

	merge_lines(gpu, y);
}

static void
setup_windows(gpu_2d_engine *gpu)
{
	gpu->window_enabled[0] = gpu->dispcnt & BIT(13);
	gpu->window_enabled[1] = gpu->dispcnt & BIT(14);
	gpu->window_enabled[2] = gpu->dispcnt & BIT(15);
	gpu->window_any_enabled = gpu->window_enabled[0] ||
	                          gpu->window_enabled[1] ||
	                          gpu->window_enabled[2];

	gpu->window_bits[0] = gpu->winin;
	gpu->window_bits[1] = gpu->winin >> 8;
	gpu->window_bits[2] = gpu->winout >> 8;
	gpu->window_bits[3] = gpu->winout;
	gpu->active_window.fill(3);
}

static void
set_active_window(gpu_2d_engine *gpu)
{
	if (!gpu->window_any_enabled) {
		gpu->window_bits_line.fill(0x3F);
		return;
	}

	for (u32 w = 2; w--;) {
		if (!gpu->window_enabled[w])
			continue;

		if (!gpu->window_y_in_range[w])
			continue;

		u32 start = gpu->win_h[w] >> 8;
		u32 end = gpu->win_h[w] & 0xFF;

		if (start > end || (end == 0 && start != 0)) {
			end = 256;
		}

		for (u32 x = start; x < end; x++) {
			gpu->active_window[x] = w;
		}
	}

	for (u32 x = 0; x < 256; x++) {
		int w = gpu->active_window[x];
		gpu->window_bits_line[x] = gpu->window_bits[w];
	}
}

static void
apply_window_to_obj_line(gpu_2d_engine *gpu)
{
	if (!gpu->window_any_enabled)
		return;

	for (u32 x = 0; x < 256; x++) {
		if (!layer_in_window(gpu, 4, x)) {
			gpu->obj_attr[x] = BIT(31);
		}
	}
}

static void
clear_obj_buffers(gpu_2d_engine *gpu)
{
	u32 attr = (u32)0x80 << 24;
	gpu->obj_line.fill(color_u());
	gpu->obj_attr.fill(attr);
}

static void
clear_bg_buffers(gpu_2d_engine *gpu)
{
	for (u32 i = 0; i < 4; i++) {
		u32 attr = (u32)(0x80 + 4 + i) << 24;
		gpu->bg_line[i].fill(color_u());
		gpu->bg_attr[i].fill(attr);
	}
}

static void
merge_lines(gpu_2d_engine *gpu, u32)
{
	u16 backdrop_color = bg_get_color(gpu, 0);
	u32 backdrop_attr = (u32)0x40 << 24 | (gpu->bldcnt >> 5 & 0x101);
	u32 obj_blend_attr = gpu->bldcnt >> 4 & 0x101;

	color_u color[2]{};
	u32 attr[2]{};

	u8 effect = gpu->bldcnt >> 6 & 3;
	u8 eva = std::min(gpu->bldalpha & 0x1F, 16);
	u8 evb = std::min(gpu->bldalpha >> 8 & 0x1F, 16);
	u8 evy = std::min(gpu->bldy & 0x1F, 16);

	for (u32 x = 0; x < 256; x++) {
		color[0] = backdrop_color;
		color[1] = backdrop_color;
		attr[0] = backdrop_attr;
		attr[1] = backdrop_attr;

		if (gpu->obj_attr[x] < attr[0]) {
			attr[0] = gpu->obj_attr[x] | obj_blend_attr;
			color[0] = gpu->obj_line[x];
		}

		for (u32 bg = 0; bg < 4; bg++) {
			u32 bg_attr = gpu->bg_attr[bg][x];

			if (bg_attr < attr[0]) {
				attr[1] = attr[0];
				color[1] = color[0];
				attr[0] = bg_attr;
				color[0] = gpu->bg_line[bg][x];
			} else if (bg_attr < attr[1]) {
				attr[1] = bg_attr;
				color[1] = gpu->bg_line[bg][x];
			}
		}

		if (!(attr[0] & BIT(2))) {
			color[0] = unpack_bgr555_2d(color[0].v);
		}

		if (!(attr[1] & BIT(2))) {
			color[1] = unpack_bgr555_2d(color[1].v);
		}

		color4 result = color[0].p;
		bool top_fx = attr[0] & BIT(0) &&
		              gpu->window_bits_line[x] & BIT(5);
		bool bottom_fx = attr[1] & BIT(8) &&
		                 gpu->window_bits_line[x] & BIT(5);
		bool did_blend = false;

		if (attr[0] & BIT(1) && bottom_fx) {
			u8 k1, k2, shift;

			if (attr[0] & BIT(2)) {
				k1 = color[0].p.a + 1;
				k2 = 32 - k1;
				shift = 5;
			} else if (attr[0] >> 12 & 0xF) {
				k1 = (attr[0] >> 12 & 0xF) + 1;
				k2 = 16 - k1;
				shift = 4;
			} else {
				k1 = eva;
				k2 = evb;
				shift = 4;
			}

			result = alpha_blend(
					color[0].p, color[1].p, k1, k2, shift);
			did_blend = true;
		}

		switch (effect) {
		case 1:
			if (!did_blend && top_fx && bottom_fx) {
				result = alpha_blend(color[0].p, color[1].p,
						eva, evb, 4);
			}
			break;
		case 2:
			if (top_fx) {
				result = increase_brightness(color[0].p, evy);
			}
			break;
		case 3:
			if (top_fx) {
				result = decrease_brightness(color[0].p, evy);
			}
		}

		result.a = 0x1F;
		gpu->gfx_line[x] = result;
	}
}

static color4
alpha_blend(color4 c1, color4 c2, u8 k1, u8 k2, u8 shift)
{
	u8 half = 1 << (shift - 1);
	u8 r = std::min(0x3F, (c1.r * k1 + c2.r * k2 + half) >> shift);
	u8 g = std::min(0x3F, (c1.g * k1 + c2.g * k2 + half) >> shift);
	u8 b = std::min(0x3F, (c1.b * k1 + c2.b * k2 + half) >> shift);

	return { r, g, b, 0x1F };
}

static color4
increase_brightness(color4 c, u8 k)
{
	c.r += ((0x3F - c.r) * k + 8) >> 4;
	c.g += ((0x3F - c.g) * k + 8) >> 4;
	c.b += ((0x3F - c.b) * k + 8) >> 4;

	return c;
}

static color4
decrease_brightness(color4 c, u8 k)
{
	c.r -= (c.r * k + 7) >> 4;
	c.g -= (c.g * k + 7) >> 4;
	c.b -= (c.b * k + 7) >> 4;

	return c;
}

} // namespace twice
