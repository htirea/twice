#include "nds/nds.h"

#include "nds/gpu/2d/render.h"
#include "nds/mem/vram.h"

namespace twice {

static void check_internal_regs(gpu_2d_engine *gpu, u32 y);
static void update_internal_regs(gpu_2d_engine *gpu);
static void render_output_line(gpu_2d_engine *gpu, u32 y);
static void render_vram_line(gpu_2d_engine *gpu, u32 y);
static void capture_display(gpu_2d_engine *gpu, u32 y);
static void apply_master_brightness(gpu_2d_engine *gpu);
static u8 *get_lcdc_pointer_checked(nds_ctx *nds, int bank);
static u8 *get_lcdc_pointer_unchecked(nds_ctx *nds, int bank);

void
render_scanline(gpu_2d_engine *gpu, u32 y)
{
	check_internal_regs(gpu, y);

	if (gpu->enabled) {
		render_output_line(gpu, y);
		std::transform(gpu->output_line.begin(),
				gpu->output_line.end(), gpu->fb + 256 * y,
				pack_to_bgr888);
	} else {
		std::fill(gpu->fb + 256 * y, gpu->fb + 256 * (y + 1),
				0xFFFFFFFF);
	}

	update_internal_regs(gpu);
}

static void
check_internal_regs(gpu_2d_engine *gpu, u32 y)
{
	bool mosaic_boundary = y == 0 || gpu->mosaic_countdown == 0;

	if (mosaic_boundary) {
		gpu->mosaic_countup = 0;
		gpu->mosaic_countdown = (gpu->mosaic >> 4 & 0xF) + 1;
	}

	for (int i = 0; i < 2; i++) {
		bool mosaic = gpu->bg_cnt[i + 2] & BIT(6);

		if (mosaic && !mosaic_boundary)
			continue;

		gpu->bg_ref_x[i] = gpu->bg_ref_x_nm[i];
		gpu->bg_ref_y[i] = gpu->bg_ref_y_nm[i];
	}

	for (u32 i = 0; i < 2; i++) {
		if (gpu->bg_ref_x_reload[i] || y == 0) {
			gpu->bg_ref_x[i] = SEXT<28>(gpu->bg_ref_x_latch[i]);
			gpu->bg_ref_x_nm[i] = gpu->bg_ref_x[i];
			gpu->bg_ref_x_reload[i] = false;
		}

		if (gpu->bg_ref_y_reload[i] || y == 0) {
			gpu->bg_ref_y[i] = SEXT<28>(gpu->bg_ref_y_latch[i]);
			gpu->bg_ref_y_nm[i] = gpu->bg_ref_y[i];
			gpu->bg_ref_y_reload[i] = false;
		}
	}
}

static void
update_internal_regs(gpu_2d_engine *gpu)
{
	gpu->mosaic_countup++;
	gpu->mosaic_countdown--;
	gpu->bg_ref_x_nm[0] += (s16)gpu->bg_pb[0];
	gpu->bg_ref_y_nm[0] += (s16)gpu->bg_pd[0];
	gpu->bg_ref_x_nm[1] += (s16)gpu->bg_pb[1];
	gpu->bg_ref_y_nm[1] += (s16)gpu->bg_pd[1];
}

static void
render_output_line(gpu_2d_engine *gpu, u32 y)
{
	render_gfx_line(gpu, y);

	switch (gpu->dispcnt >> 16 & 3) {
	case 0:
	{
		color4 white = { 0x3F, 0x3F, 0x3F, 0x1F };
		gpu->output_line.fill(white);
		break;
	}
	case 1:
		std::copy(gpu->gfx_line.begin(), gpu->gfx_line.end(),
				gpu->output_line.begin());
		break;
	case 2:
		if (gpu->engineid == 0) {
			render_vram_line(gpu, y);
		} else {
			throw twice_error("engine B display mode 2");
		}
		break;
	case 3:
		throw twice_error("display mode 3 not implemented");
	}

	if (gpu->display_capture) {
		capture_display(gpu, y);
	}

	apply_master_brightness(gpu);
}

static void
render_vram_line(gpu_2d_engine *gpu, u32 y)
{
	u32 offset = 0x20000 * (gpu->dispcnt >> 18 & 3) + 512 * y;

	for (u32 i = 0; i < 256; i++, offset += 2) {
		gpu->output_line[i] = unpack_bgr555_2d(
				vram_read_lcdc<u16>(gpu->nds, offset));
	}
}

static void
capture_display(gpu_2d_engine *gpu, u32 y)
{
	std::pair<u32, u32> capture_dims[] = { { 128, 128 }, { 256, 64 },
		{ 256, 128 }, { 256, 192 } };
	auto [w, h] = capture_dims[gpu->dispcapcnt >> 20 & 3];

	if (y >= h)
		return;

	u8 *dest = get_lcdc_pointer_checked(
			gpu->nds, gpu->dispcapcnt >> 16 & 3);
	u32 dest_offset = y * w * 2 + 0x8000 * (gpu->dispcapcnt >> 18 & 3);
	if (!dest)
		return;

	u32 capture_src = gpu->dispcapcnt >> 29 & 3;
	color4 *src_a = gpu->dispcapcnt & BIT(24)
	                                ? gpu->nds->gpu3d.re.color_buf[0][y]
	                                                  .data()
	                                : gpu->gfx_line.data();
	u16 src_b[256]{};

	if (capture_src != 0) {
		if (gpu->dispcapcnt & BIT(25)) {
			LOG("capture from main memory fifo not implemented\n");
			return;
		} else {
			u8 *p = get_lcdc_pointer_unchecked(
					gpu->nds, gpu->dispcnt >> 18 & 3);
			u32 offset = y * w * 2;

			if ((gpu->dispcnt >> 16 & 3) != 2) {
				offset += 0x8000 * (gpu->dispcapcnt >> 26 & 3);
			}

			for (u32 i = 0; i < w; i++, offset += 2) {
				src_b[i] = readarr<u16>(p, offset & 0x1FFFF);
			}
		}
	}

	switch (capture_src) {
	case 0:
		for (u32 i = 0; i < w; i++, dest_offset += 2) {
			u16 r = src_a[i].r >> 1;
			u16 g = src_a[i].g >> 1;
			u16 b = src_a[i].b >> 1;
			u16 a = src_a[i].a != 0;
			u16 c = a << 15 | b << 10 | g << 5 | r;
			writearr<u16>(dest, dest_offset & 0x1FFFF, c);
		}
		break;
	case 1:
		for (u32 i = 0; i < w; i++, dest_offset += 2) {
			writearr<u16>(dest, dest_offset & 0x1FFFF, src_b[i]);
		}
		break;
	case 2:
	case 3:
		u8 k1 = std::min<u32>(gpu->dispcapcnt & 0x1F, 16);
		u8 k2 = std::min<u32>(gpu->dispcapcnt >> 8 & 0x1F, 16);

		for (u32 i = 0; i < w; i++, dest_offset += 2) {
			u16 r1 = src_a[i].r >> 1;
			u16 g1 = src_a[i].g >> 1;
			u16 b1 = src_a[i].b >> 1;
			u16 a1 = src_a[i].a != 0;

			u16 r2 = src_b[i] & 0x1F;
			u16 g2 = src_b[i] >> 5 & 0x1F;
			u16 b2 = src_b[i] >> 10 & 0x1F;
			u16 a2 = src_b[i] >> 15 != 0;

			u16 r = (r1 * a1 * k1 + r2 * a2 * k2 + 8) >> 4;
			u16 g = (g1 * a1 * k1 + g2 * a2 * k2 + 8) >> 4;
			u16 b = (b1 * a1 * k1 + b2 * a2 * k2 + 8) >> 4;
			u16 a = (a1 && k1) || (a2 && k2);

			r = std::min<u8>(r, 0x1F);
			g = std::min<u8>(g, 0x1F);
			b = std::min<u8>(b, 0x1F);

			u16 c = a << 15 | b << 10 | g << 5 | r;
			writearr<u16>(dest, dest_offset & 0x1FFFF, c);
		}
	}
}

static void
apply_master_brightness(gpu_2d_engine *gpu)
{
	u8 k = std::min(gpu->master_bright & 0x1F, 16);

	switch (gpu->master_bright >> 14 & 3) {
	case 1:
		for (auto& c : gpu->output_line) {
			c.r += (63 - c.r) * k >> 4;
			c.g += (63 - c.g) * k >> 4;
			c.b += (63 - c.b) * k >> 4;
		}
		break;
	case 2:
		for (auto& c : gpu->output_line) {
			c.r -= c.r * k >> 4;
			c.g -= c.g * k >> 4;
			c.b -= c.b * k >> 4;
		}
	}
}

static u8 *
get_lcdc_pointer_checked(nds_ctx *nds, int bank)
{
	return (nds->vram.vramcnt[bank] & 7) == 0
	                       ? get_lcdc_pointer_unchecked(nds, bank)
	                       : nullptr;
}

static u8 *
get_lcdc_pointer_unchecked(nds_ctx *nds, int bank)
{
	return nds->vram.bank_to_base_ptr[bank];
}

} // namespace twice
