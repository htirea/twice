#ifndef TWICE_GPU3D_RE_H
#define TWICE_GPU3D_RE_H

#include "common/types.h"
#include "nds/gpu/3d/polygon.h"
#include "nds/gpu/color.h"

namespace twice {

struct gpu_3d_engine;

struct rendering_engine {
	struct registers {
		u16 disp3dcnt{};
		u32 clear_color{};
		u16 clear_depth{};
		u16 clrimage_offset{};
		std::array<u16, 32> _toon_table{};
		std::array<u16, 8> _edge_color{};
		std::array<u8, 32> fog_table{};
		u32 fog_color{};
		u16 fog_offset{};
		u8 alpha_test_ref{};

		bool operator==(const registers&) const = default;
	};

	registers r, r_s;
	bool manual_sort{};

	color4 color_buf[2][192][256]{};
	s32 depth_buf[2][192][256]{};
	bool stencil_buf[2][256]{};

	vertex_ram *vtx_ram{};
	polygon_ram *poly_ram{};
	gpu_3d_engine *gpu{};
};

void re_render_frame(rendering_engine *re);

} // namespace twice

#endif
