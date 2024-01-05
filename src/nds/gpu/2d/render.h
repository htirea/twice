#ifndef TWICE_GPU2D_RENDER_H
#define TWICE_GPU2D_RENDER_H

#include "common/types.h"
#include "nds/nds.h"

namespace twice {

void render_scanline(gpu_2d_engine *gpu, u32 y);
void render_gfx_line(gpu_2d_engine *gpu, u32 y);
void render_obj_line(gpu_2d_engine *gpu, u32 y);
void render_text_bg_line(gpu_2d_engine *gpu, int bg_id, u32 y);
void render_affine_bg_line(gpu_2d_engine *gpu, int bg_id, u32 y);
void render_extended_bg_line(gpu_2d_engine *gpu, int bg_id, u32 y);
void render_ext_text_bg_line(gpu_2d_engine *gpu, int bg_id, u32 y);
void render_ext_bitmap_bg_line(
		gpu_2d_engine *gpu, int bg_id, u32 y, bool direct_color);
void render_large_bmp_bg_line(gpu_2d_engine *gpu, u32 y);
void render_3d_line(gpu_2d_engine *gpu, u32 y);

inline u16
bg_get_color(gpu_2d_engine *gpu, u32 color_num)
{
	return readarr<u16>(gpu->nds->palette,
			gpu->palette_offset + 2 * color_num);
}

inline bool
layer_in_window(gpu_2d_engine *gpu, u32 layer, u32 x)
{
	return gpu->window_bits_line[x] & BIT(layer);
}

} // namespace twice

#endif
