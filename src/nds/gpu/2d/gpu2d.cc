#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/gpu/color.h"
#include "nds/mem/vram.h"

#include "common/logger.h"
#include "libtwice/exception.h"

namespace twice {

static void update_window_y_in_range(gpu_2d_engine *gpu, u16 scanline);
static void update_mosaic_and_ref_xy(gpu_2d_engine *gpu, u16 y);
static void update_mosaic_counters(gpu_2d_engine *gpu);
static void draw_scanline(gpu_2d_engine *gpu, u16 scanline);

void
gpu_on_scanline_start(nds_ctx *nds)
{
	update_window_y_in_range(&nds->gpu2d[0], nds->vcount);
	update_window_y_in_range(&nds->gpu2d[1], nds->vcount);

	if (nds->vcount == 0 && nds->gpu2d[0].dispcapcnt & BIT(31)) {
		nds->gpu2d[0].display_capture = true;
	}

	if (nds->vcount < 192) {
		update_mosaic_and_ref_xy(&nds->gpu2d[0], nds->vcount);
		update_mosaic_and_ref_xy(&nds->gpu2d[1], nds->vcount);

		draw_scanline(&nds->gpu2d[0], nds->vcount);
		draw_scanline(&nds->gpu2d[1], nds->vcount);

		update_mosaic_counters(&nds->gpu2d[0]);
		update_mosaic_counters(&nds->gpu2d[1]);
	}

	if (nds->vcount == 192 && nds->gpu2d[0].display_capture) {
		nds->gpu2d[0].display_capture = false;
		nds->gpu2d[0].dispcapcnt &= ~BIT(31);
	}
}

static void
update_window_y_in_range(gpu_2d_engine *gpu, u16 scanline)
{
	for (int i = 0; i < 2; i++) {
		u8 y1 = gpu->win_v[i] >> 8;
		u8 y2 = gpu->win_v[i];

		if ((scanline & 0xFF) == y1) {
			gpu->window_y_in_range[i] = true;
		}
		if ((scanline & 0xFF) == y2) {
			gpu->window_y_in_range[i] = false;
		}
	}
}

static void
update_mosaic_and_ref_xy(gpu_2d_engine *gpu, u16 y)
{
	s32 mosaic_step = y == 0 ? 0 : gpu->mosaic_countup;
	bool boundary = y == 0 || gpu->mosaic_countdown == 0;

	if (boundary) {
		gpu->mosaic_countup = 0;
		gpu->mosaic_countdown = (gpu->mosaic >> 4 & 0xF) + 1;
	}

	for (int i = 0; i < 2; i++) {
		gpu->bg_ref_x_nonmosaic[i] += (s16)gpu->bg_pb[i];
		gpu->bg_ref_y_nonmosaic[i] += (s16)gpu->bg_pd[i];

		bool mosaic = gpu->bg_cnt[i + 2] & BIT(6);
		if (mosaic && !boundary)
			continue;

		if (mosaic) {
			gpu->bg_ref_x[i] += mosaic_step * (s16)gpu->bg_pb[i];
			gpu->bg_ref_y[i] += mosaic_step * (s16)gpu->bg_pd[i];
		} else {
			gpu->bg_ref_x[i] = gpu->bg_ref_x_nonmosaic[i];
			gpu->bg_ref_y[i] = gpu->bg_ref_y_nonmosaic[i];
		}
	}

	for (int i = 0; i < 2; i++) {
		if (gpu->bg_ref_x_reload[i] || y == 0) {
			gpu->bg_ref_x[i] = SEXT<28>(gpu->bg_ref_x_latch[i]);
			gpu->bg_ref_x_nonmosaic[i] = gpu->bg_ref_x[i];
			gpu->bg_ref_x_reload[i] = false;
		}
		if (gpu->bg_ref_y_reload[i] || y == 0) {
			gpu->bg_ref_y[i] = SEXT<28>(gpu->bg_ref_y_latch[i]);
			gpu->bg_ref_y_nonmosaic[i] = gpu->bg_ref_y[i];
			gpu->bg_ref_y_reload[i] = false;
		}
	}
}

static void
update_mosaic_counters(gpu_2d_engine *gpu)
{
	gpu->mosaic_countup++;
	gpu->mosaic_countdown--;
}

static void fb_fill_white(u32 *fb, u16 y);
static void graphics_display_scanline(gpu_2d_engine *gpu);
static void output_fill_white(gpu_2d_engine *gpu);
static void vram_display_scanline(gpu_2d_engine *gpu);
static void capture_display(gpu_2d_engine *gpu, u16 y);
static void adjust_master_brightness(gpu_2d_engine *gpu);
static void write_output_to_fb(gpu_2d_engine *gpu, u16 y);

static void
draw_scanline(gpu_2d_engine *gpu, u16 scanline)
{
	if (!gpu->enabled) {
		fb_fill_white(gpu->fb, scanline);
		return;
	}

	graphics_display_scanline(gpu);

	switch (gpu->dispcnt >> 16 & 0x3) {
	case 0:
		output_fill_white(gpu);
		break;
	case 1:
		std::copy(gpu->gfx_line, gpu->gfx_line + 256,
				gpu->output_line);
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

	if (gpu->display_capture) {
		capture_display(gpu, scanline);
	}

	adjust_master_brightness(gpu);
	write_output_to_fb(gpu, scanline);
}

static void
fb_fill_white(u32 *fb, u16 y)
{
	u32 start = y * 256;
	u32 end = start + 256;

	for (u32 i = start; i < end; i++) {
		fb[i] = 0xFFFFFFFF;
	}
}

static void setup_window_info(gpu_2d_engine *gpu);
static void clear_obj_buffer(gpu_2d_engine *gpu);
static void set_active_window(gpu_2d_engine *gpu);
static void set_backdrop(gpu_2d_engine *gpu);
static void push_obj_pixel(gpu_2d_engine *gpu, u32 i);
static color4 pixel_to_color(gpu_2d_engine::pixel p);
static color4 alpha_blend(
		color4 color1, color4 color2, u8 eva, u8 evb, u8 shift);
static color4 increase_brightness(color4 color, u8 evy);
static color4 decrease_brightness(color4 color, u8 evy);

void render_sprites(gpu_2d_engine *gpu);
u16 bg_get_color_256(gpu_2d_engine *gpu, u32 color_num);
void render_text_bg(gpu_2d_engine *gpu, int bg);
void render_affine_bg(gpu_2d_engine *gpu, int bg);
void render_extended_bg(gpu_2d_engine *gpu, int bg);
void render_large_bitmap_bg(gpu_2d_engine *gpu);
void render_3d(gpu_2d_engine *gpu);

static void
graphics_display_scanline(gpu_2d_engine *gpu)
{
	auto& dispcnt = gpu->dispcnt;

	setup_window_info(gpu);

	clear_obj_buffer(gpu);
	if (dispcnt & BIT(12)) {
		render_sprites(gpu);
	}

	set_active_window(gpu);
	set_backdrop(gpu);

	switch (dispcnt & 0x7) {
	case 0:
		if (dispcnt & BIT(8))
			render_text_bg(gpu, 0);
		if (dispcnt & BIT(9))
			render_text_bg(gpu, 1);
		if (dispcnt & BIT(10))
			render_text_bg(gpu, 2);
		if (dispcnt & BIT(11))
			render_text_bg(gpu, 3);
		break;
	case 1:
		if (dispcnt & BIT(8))
			render_text_bg(gpu, 0);
		if (dispcnt & BIT(9))
			render_text_bg(gpu, 1);
		if (dispcnt & BIT(10))
			render_text_bg(gpu, 2);
		if (dispcnt & BIT(11))
			render_affine_bg(gpu, 3);
		break;
	case 2:
		if (dispcnt & BIT(8))
			render_text_bg(gpu, 0);
		if (dispcnt & BIT(9))
			render_text_bg(gpu, 1);
		if (dispcnt & BIT(10))
			render_affine_bg(gpu, 2);
		if (dispcnt & BIT(11))
			render_affine_bg(gpu, 3);
		break;
	case 3:
		if (dispcnt & BIT(8))
			render_text_bg(gpu, 0);
		if (dispcnt & BIT(9))
			render_text_bg(gpu, 1);
		if (dispcnt & BIT(10))
			render_text_bg(gpu, 2);
		if (dispcnt & BIT(11))
			render_extended_bg(gpu, 3);
		break;
	case 4:
		if (dispcnt & BIT(8))
			render_text_bg(gpu, 0);
		if (dispcnt & BIT(9))
			render_text_bg(gpu, 1);
		if (dispcnt & BIT(10))
			render_affine_bg(gpu, 2);
		if (dispcnt & BIT(11))
			render_extended_bg(gpu, 3);
		break;
	case 5:
		if (dispcnt & BIT(8))
			render_text_bg(gpu, 0);
		if (dispcnt & BIT(9))
			render_text_bg(gpu, 1);
		if (dispcnt & BIT(10))
			render_extended_bg(gpu, 2);
		if (dispcnt & BIT(11))
			render_extended_bg(gpu, 3);
		break;
	case 6:
		if (gpu->engineid == 1) {
			throw twice_error("bg mode 6 invalid for engine B");
		}
		if (dispcnt & BIT(8))
			render_3d(gpu);
		if (dispcnt & BIT(10))
			render_large_bitmap_bg(gpu);
		break;
	case 7:
		throw twice_error("bg mode 7 invalid");
	}

	bool obj_effect_top = gpu->bldcnt & BIT(4);
	bool obj_effect_bottom = gpu->bldcnt & BIT(12);

	if (gpu->window_any_enabled) {
		for (u32 i = 0; i < 256; i++) {
			u32 w = gpu->window_buffer[i].window;
			u8 bits = gpu->window[w].enable_bits;
			if (bits & BIT(4)) {
				auto& obj = gpu->obj_buffer[i];
				bool effects = bits & BIT(5);
				obj.effect_top = obj_effect_top && effects;
				obj.effect_bottom =
						obj_effect_bottom && effects;
				push_obj_pixel(gpu, i);
			}
		}
	} else {
		for (u32 i = 0; i < 256; i++) {
			auto& obj = gpu->obj_buffer[i];
			obj.effect_top = obj_effect_top;
			obj.effect_bottom = obj_effect_bottom;
			push_obj_pixel(gpu, i);
		}
	}

	u8 effect = gpu->bldcnt >> 6 & 3;
	u8 eva = std::min(gpu->bldalpha & 0x1F, 16);
	u8 evb = std::min(gpu->bldalpha >> 8 & 0x1F, 16);
	u8 evy = std::min(gpu->bldy & 0x1F, 16);

	for (u32 i = 0; i < 256; i++) {
		auto& top = gpu->buffer_top[i];
		auto& bottom = gpu->buffer_bottom[i];

		color4 top_color = pixel_to_color(top);
		color4 bottom_color = pixel_to_color(bottom);
		color4 color_result = top_color;

		bool blended = false;
		if (top.force_blend && bottom.effect_bottom) {
			u8 ta, tb, shift;
			if (top.from_3d) {
				ta = top.color.p.a + 1;
				tb = 32 - ta;
				shift = 5;
			} else if (top.alpha_oam) {
				ta = top.alpha_oam + 1;
				tb = 16 - ta;
				shift = 4;
			} else {
				ta = eva;
				tb = evb;
				shift = 4;
			}
			color_result = alpha_blend(top_color, bottom_color, ta,
					tb, shift);
			blended = true;
		}

		switch (effect) {
		case 1:
			if (!blended && top.effect_top &&
					bottom.effect_bottom) {
				color_result = alpha_blend(top_color,
						bottom_color, eva, evb, 4);
			}
			break;
		case 2:
			if (top.effect_top) {
				color_result = increase_brightness(
						top_color, evy);
			}
			break;
		case 3:
			if (top.effect_top) {
				color_result = decrease_brightness(
						top_color, evy);
			}
			break;
		}

		color_result.a = 1;
		gpu->gfx_line[i] = color_result;
	}
}

static void
setup_window_info(gpu_2d_engine *gpu)
{
	for (u32 i = 0; i < 256; i++) {
		gpu->window_buffer[i].window = 3;
	}

	gpu->window[0].enable_bits = gpu->winin;
	gpu->window[1].enable_bits = gpu->winin >> 8;
	gpu->window[2].enable_bits = gpu->winout >> 8;
	gpu->window[3].enable_bits = gpu->winout;

	gpu->window_enabled[0] = gpu->dispcnt & BIT(13);
	gpu->window_enabled[1] = gpu->dispcnt & BIT(14);
	gpu->window_enabled[2] = gpu->dispcnt & BIT(15);
	gpu->window_any_enabled = gpu->window_enabled[0] ||
	                          gpu->window_enabled[1] ||
	                          gpu->window_enabled[2];
}

static void
clear_obj_buffer(gpu_2d_engine *gpu)
{
	for (u32 i = 0; i < 256; i++) {
		gpu->obj_buffer[i].color = 0;
		gpu->obj_buffer[i].priority = 7;
		gpu->obj_buffer[i].effect_top = 0;
		gpu->obj_buffer[i].effect_bottom = 0;
		gpu->obj_buffer[i].force_blend = 0;
		gpu->obj_buffer[i].alpha_oam = 0;
		gpu->obj_buffer[i].from_3d = 0;
	}
}

static void
set_active_window(gpu_2d_engine *gpu)
{
	for (int w = 1; w >= 0; w--) {
		if (!gpu->window_enabled[w])
			continue;
		if (!gpu->window_y_in_range[w])
			continue;

		u32 x1 = gpu->win_h[w] >> 8;
		u32 x2 = gpu->win_h[w] & 0xFF;
		if (x1 > x2 || (x2 == 0 && x1 != 0)) {
			x2 = 256;
		}
		for (u32 i = x1; i < x2; i++) {
			gpu->window_buffer[i].window = w;
		}
	}
}

static void
set_backdrop(gpu_2d_engine *gpu)
{
	u16 color = bg_get_color_256(gpu, 0);
	bool effect_top = gpu->bldcnt & BIT(5);
	bool effect_bottom = gpu->bldcnt & BIT(13);

	if (gpu->window_any_enabled) {
		for (u32 i = 0; i < 256; i++) {
			u32 w = gpu->window_buffer[i].window;
			bool wfx = gpu->window[w].enable_bits & BIT(5);

			gpu->buffer_top[i].color = color;
			gpu->buffer_top[i].priority = 4;
			gpu->buffer_top[i].effect_top = effect_top & wfx;
			gpu->buffer_top[i].effect_bottom = effect_bottom & wfx;
			gpu->buffer_top[i].force_blend = 0;
			gpu->buffer_top[i].alpha_oam = 0;
			gpu->buffer_top[i].from_3d = 0;
			gpu->buffer_bottom[i] = gpu->buffer_top[i];
		}
	} else {
		for (u32 i = 0; i < 256; i++) {
			gpu->buffer_top[i].color = color;
			gpu->buffer_top[i].priority = 4;
			gpu->buffer_top[i].effect_top = effect_top;
			gpu->buffer_top[i].effect_bottom = effect_bottom;
			gpu->buffer_top[i].force_blend = 0;
			gpu->buffer_top[i].alpha_oam = 0;
			gpu->buffer_top[i].from_3d = 0;
			gpu->buffer_bottom[i] = gpu->buffer_top[i];
		}
	}
}

static void
push_obj_pixel(gpu_2d_engine *gpu, u32 i)
{
	auto& obj = gpu->obj_buffer[i];

	if (obj.priority <= gpu->buffer_top[i].priority) {
		gpu->buffer_bottom[i] = gpu->buffer_top[i];
		gpu->buffer_top[i] = obj;
	} else if (obj.priority <= gpu->buffer_bottom[i].priority) {
		gpu->buffer_bottom[i] = obj;
	}
}

static color4
pixel_to_color(gpu_2d_engine::pixel p)
{
	if (p.from_3d) {
		return p.color.p;
	} else {
		return unpack_bgr555_2d(p.color.v);
	}
}

static color4
alpha_blend(color4 color1, color4 color2, u8 eva, u8 evb, u8 shift)
{
	u8 r = std::min(63, (color1.r * eva + color2.r * evb + 8) >> shift);
	u8 g = std::min(63, (color1.g * eva + color2.g * evb + 8) >> shift);
	u8 b = std::min(63, (color1.b * eva + color2.b * evb + 8) >> shift);

	return { r, g, b, 0xFF };
}

static color4
increase_brightness(color4 color, u8 evy)
{
	color.r += ((63 - color.r) * evy + 8) >> 4;
	color.g += ((63 - color.g) * evy + 8) >> 4;
	color.b += ((63 - color.b) * evy + 8) >> 4;

	return color;
}

static color4
decrease_brightness(color4 color, u8 evy)
{
	color.r -= (color.r * evy + 7) >> 4;
	color.g -= (color.g * evy + 7) >> 4;
	color.b -= (color.b * evy + 7) >> 4;

	return color;
}

static void
output_fill_white(gpu_2d_engine *gpu)
{
	for (u32 i = 0; i < 256; i++) {
		gpu->output_line[i] = { 0x3F, 0x3F, 0x3F };
	}
}

static void
vram_display_scanline(gpu_2d_engine *gpu)
{
	u32 offset = 0x20000 * (gpu->dispcnt >> 18 & 0x3) +
	             gpu->nds->vcount * 256 * 2;

	for (u32 i = 0; i < 256; i++) {
		gpu->output_line[i] = unpack_bgr555_2d(
				vram_read_lcdc<u16>(gpu->nds, offset + i * 2));
	}
}

static void
capture_display(gpu_2d_engine *gpu, u16 y)
{
	u32 widths[] = { 128, 256, 256, 256 };
	u32 heights[] = { 128, 64, 128, 192 };
	u32 w = widths[gpu->dispcapcnt >> 20 & 3];
	u32 h = heights[gpu->dispcapcnt >> 20 & 3];

	if (y >= h)
		return;

	u32 write_bank = gpu->dispcapcnt >> 16 & 3;
	u8 *write_p = gpu->nds->vram.bank_to_base_ptr[write_bank];
	u32 write_offset = y * w * 2 + 0x8000 * (gpu->dispcapcnt >> 18 & 3);

	if ((gpu->nds->vram.vramcnt[write_bank] & 7) != 0)
		return;

	color4 *src_a{};
	u16 src_b[256]{};

	u32 source = gpu->dispcapcnt >> 29 & 3;

	if (source != 1) {
		if (gpu->dispcapcnt & BIT(24)) {
			src_a = gpu->nds->gpu3d.color_buf[0][y];
		} else {
			src_a = gpu->gfx_line;
		}
	}

	if (source != 0) {
		if (gpu->dispcapcnt & BIT(25)) {
			LOG("capture from main memory fifo not implemented\n");
			return;
		} else {
			u32 read_bank = gpu->dispcnt >> 18 & 3;
			u32 read_offset = y * w * 2;
			if ((gpu->dispcnt >> 16 & 3) != 2) {
				read_offset += 0x8000 *
				               (gpu->dispcapcnt >> 26 & 3);
			}
			u8 *p = gpu->nds->vram.bank_to_base_ptr[read_bank];
			for (u32 i = 0; i < w; i++) {
				u32 offset = (read_offset + i * 2) & 0x1FFFF;
				src_b[i] = readarr<u16>(p, offset);
			}
		}
	}

	switch (source) {
	case 0:
	{
		for (u32 i = 0; i < w; i++) {
			u16 r = src_a[i].r >> 1;
			u16 g = src_a[i].g >> 1;
			u16 b = src_a[i].b >> 1;
			u16 a = src_a[i].a != 0;

			u16 color = a << 15 | b << 10 | g << 5 | r;
			u32 offset = (write_offset + i * 2) & 0x1FFFF;
			writearr<u16>(write_p, offset, color);
		}
		break;
	}
	case 1:
	{
		for (u32 i = 0; i < w; i++) {
			u32 offset = (write_offset + i * 2) & 0x1FFFF;
			writearr<u16>(write_p, offset, src_b[i]);
		}
		break;
	}
	case 2:
	case 3:
	{
		u8 eva = std::min(gpu->dispcapcnt & 0x1F, (u32)16);
		u8 evb = std::min(gpu->dispcapcnt >> 8 & 0x1F, (u32)16);

		for (u32 i = 0; i < w; i++) {
			u8 ra = src_a[i].r >> 1;
			u8 ga = src_a[i].g >> 1;
			u8 ba = src_a[i].b >> 1;
			u8 aa = src_a[i].a != 0;

			u8 rb = src_b[i] & 0x1F;
			u8 gb = src_b[i] >> 5 & 0x1F;
			u8 bb = src_b[i] >> 10 & 0x1F;
			u8 ab = src_b[i] >> 15 != 0;

			u16 r = (ra * aa * eva + rb * ab * evb + 8) >> 4;
			u16 g = (ga * aa * eva + gb * ab * evb + 8) >> 4;
			u16 b = (ba * aa * eva + bb * ab * evb + 8) >> 4;
			u16 a = (aa && eva) || (ba && evb);

			r = std::min((u16)31, r);
			g = std::min((u16)31, g);
			b = std::min((u16)31, b);

			u16 color = a << 15 | b << 10 | g << 5 | r;

			u32 offset = (write_offset + i * 2) & 0x1FFFF;
			writearr<u16>(write_p, offset, color);
		}
	}
	}
}

static void
adjust_master_brightness(gpu_2d_engine *gpu)
{
	u8 factor = std::min(gpu->master_bright & 0x1F, 16);
	u8 mode = gpu->master_bright >> 14 & 3;

	switch (mode) {
	case 1:
		for (u32 i = 0; i < 256; i++) {
			color4 *color = &gpu->output_line[i];
			color->r += (63 - color->r) * factor >> 4;
			color->g += (63 - color->g) * factor >> 4;
			color->b += (63 - color->b) * factor >> 4;
		}
		break;
	case 2:
		for (u32 i = 0; i < 256; i++) {
			color4 *color = &gpu->output_line[i];
			color->r -= color->r * factor >> 4;
			color->g -= color->g * factor >> 4;
			color->b -= color->b * factor >> 4;
		}
		break;
	}
}

static void
write_output_to_fb(gpu_2d_engine *gpu, u16 y)
{
	u32 fb_offset = y * 256;

	for (u32 i = 0; i < 256; i++) {
		gpu->fb[fb_offset + i] = pack_to_bgr888(gpu->output_line[i]);
	}
}

u8
gpu_2d_read8(gpu_2d_engine *, u8 offset)
{
	LOG("2d engine read 8 at offset %02X\n", offset);
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
	case 0x10:
		return gpu->bg_hofs[0];
	case 0x12:
		return gpu->bg_vofs[0];
	case 0x14:
		return gpu->bg_hofs[1];
	case 0x16:
		return gpu->bg_vofs[1];
	case 0x18:
		return gpu->bg_hofs[2];
	case 0x1A:
		return gpu->bg_vofs[2];
	case 0x1C:
		return gpu->bg_hofs[3];
	case 0x1E:
		return gpu->bg_vofs[3];
	case 0x44:
		return gpu->win_v[0];
	case 0x46:
		return gpu->win_v[1];
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

u32
gpu_2d_read32(gpu_2d_engine *gpu, u8 offset)
{
	switch (offset) {
	case 0x8:
		return (u32)gpu->bg_cnt[1] << 16 | gpu->bg_cnt[0];
	case 0xC:
		return (u32)gpu->bg_cnt[3] << 16 | gpu->bg_cnt[2];
	case 0x10:
		return (u32)gpu->bg_vofs[0] << 16 | gpu->bg_hofs[0];
	case 0x14:
		return (u32)gpu->bg_vofs[1] << 16 | gpu->bg_hofs[1];
	case 0x18:
		return (u32)gpu->bg_vofs[2] << 16 | gpu->bg_hofs[2];
	case 0x1C:
		return (u32)gpu->bg_vofs[3] << 16 | gpu->bg_hofs[3];
	case 0x48:
		return (u32)gpu->winout << 16 | gpu->winin;
	}

	LOG("2d engine read 32 at offset %02X\n", offset);
	return 0;
}

void
gpu_2d_write8(gpu_2d_engine *gpu, u8 offset, u8 value)
{
	/* TODO: check if disabled */

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
	case 0x4C:
		gpu->mosaic = (gpu->mosaic & 0xFF00) | value;
		return;
	case 0x4D:
		gpu->mosaic = (gpu->mosaic & 0x00FF) | (u16)value << 8;
		return;
	}

	LOG("2d engine write 8 to offset %08X\n", offset);
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
	case 0x28:
		gpu->bg_ref_x_latch[0] &= ~0xFFFF;
		gpu->bg_ref_x_latch[0] |= value;
		gpu->bg_ref_x_reload[0] = true;
		return;
	case 0x2A:
		gpu->bg_ref_x_latch[0] &= 0xFFFF;
		gpu->bg_ref_x_latch[0] |= (u32)value << 16;
		gpu->bg_ref_x_reload[0] = true;
		return;
	case 0x2C:
		gpu->bg_ref_y_latch[0] &= ~0xFFFF;
		gpu->bg_ref_y_latch[0] |= value;
		gpu->bg_ref_y_reload[0] = true;
		return;
	case 0x2E:
		gpu->bg_ref_y_latch[0] &= 0xFFFF;
		gpu->bg_ref_y_latch[0] |= (u32)value << 16;
		gpu->bg_ref_y_reload[0] = true;
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
	case 0x38:
		gpu->bg_ref_x_latch[1] &= ~0xFFFF;
		gpu->bg_ref_x_latch[1] |= value;
		gpu->bg_ref_x_reload[1] = true;
		return;
	case 0x3A:
		gpu->bg_ref_x_latch[1] &= 0xFFFF;
		gpu->bg_ref_x_latch[1] |= (u32)value << 16;
		gpu->bg_ref_x_reload[1] = true;
		return;
	case 0x3C:
		gpu->bg_ref_y_latch[1] &= ~0xFFFF;
		gpu->bg_ref_y_latch[1] |= value;
		gpu->bg_ref_y_reload[1] = true;
		return;
	case 0x3E:
		gpu->bg_ref_y_latch[1] &= 0xFFFF;
		gpu->bg_ref_y_latch[1] |= (u32)value << 16;
		gpu->bg_ref_y_reload[1] = true;
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
		gpu->bldy = value & 0x1F;
		return;
	}

	LOG("2d engine write 16 to offset %02X\n", offset);
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
		gpu->bldy = value & 0x1F;
		return;
	}

	gpu_2d_write16(gpu, offset, value);
	gpu_2d_write16(gpu, offset + 2, value >> 16);
}

} // namespace twice
