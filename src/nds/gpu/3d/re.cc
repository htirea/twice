#include "nds/nds.h"

namespace twice {

static void
render_3d_scanline(gpu_3d_engine *gpu, u32 scanline)
{
	auto& vr = gpu->vtx_ram[gpu->re_buf];

	s32 viewport_w = gpu->viewport_x[1] - gpu->viewport_x[0];
	s32 viewport_h = gpu->viewport_y[1] - gpu->viewport_y[0];

	for (u32 i = 0; i < vr.count; i++) {
		auto& v = vr.vertices[i];

		if (v.w == 0)
			continue;

		s32 sx = (v.x + v.w) * viewport_w / (2 * v.w) +
		         gpu->viewport_x[0];
		s32 sy = (v.y + v.w) * viewport_h / (2 * v.w) +
		         gpu->viewport_y[0];
		sy = 192 - sy;

		if (sx < 0 || sx >= 256)
			continue;

		if (sy == (s32)scanline) {
			gpu->color_buf[sy][sx] = MASK(23);
		}
	}
}

static u32
axbgr_51555_to_abgr5666(u32 s)
{
	u32 r = s & 0x1F;
	u32 g = s >> 5 & 0x1F;
	u32 b = s >> 10 & 0x1F;
	u32 a = s >> 16 & 0x1F;

	if (r)
		r = (r << 1) + 1;
	if (g)
		g = (g << 1) + 1;
	if (b)
		b = (b << 1) + 1;

	return a << 18 | b << 12 | g << 6 | r;
}

static void
clear_buffers(gpu_3d_engine *gpu)
{
	auto& re = gpu->re;

	if (!(re.r.disp3dcnt & BIT(14))) {
		u32 color = axbgr_51555_to_abgr5666(re.r.clear_color);
		u32 attr = re.r.clear_color & 0x3F008000;
		u32 depth = 0x200 * re.r.clear_depth + 0x1FF;

		for (u32 i = 0; i < 192; i++) {
			for (u32 j = 0; j < 256; j++) {
				gpu->color_buf[i][j] = color;
				gpu->depth_buf[i][j] = depth;
				gpu->attr_buf[i][j] = attr;
			}
		}
	} else {
		/* TODO: */
	}
}

void
gpu3d_render_frame(gpu_3d_engine *gpu)
{
	clear_buffers(gpu);

	for (u32 i = 0; i < 192; i++) {
		render_3d_scanline(gpu, i);
	}
}

} // namespace twice
