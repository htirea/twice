#include "nds/nds.h"

namespace twice {

static void
render_3d_scanline(gpu_3d_engine *gpu, u32 scanline)
{
	auto& vr = gpu->vtx_ram[gpu->re_buf];

	for (u32 i = 0; i < 256; i++) {
		gpu->color_buffer[scanline][i] = 0;
	}

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
			gpu->color_buffer[sy][sx] = MASK(23);
		}
	}
}

void
gpu3d_render_frame(gpu_3d_engine *gpu)
{
	for (u32 i = 0; i < 192; i++) {
		render_3d_scanline(gpu, i);
	}
}

} // namespace twice
