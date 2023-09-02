#include "nds/nds.h"

namespace twice {

using vertex = gpu_3d_engine::vertex;
using polygon = gpu_3d_engine::polygon;

static u32
color6_to_abgr666(color6 color)
{
	return (u32)(0x1F) << 18 | (u32)color.b << 12 | (u32)color.g << 6 |
	       (u32)color.r;
}

static void
render_3d_scanline(gpu_3d_engine *gpu, u32 scanline)
{
	auto& vr = gpu->vtx_ram[gpu->re_buf];

	for (u32 i = 0; i < vr.count; i++) {
		vertex *v = &vr.vertices[i];

		if (v->sx < 0 || v->sx >= 256)
			continue;

		if (v->sy == (s32)scanline) {
			gpu->color_buf[v->sy][v->sx] =
					color6_to_abgr666(v->color);
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
