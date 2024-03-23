#ifndef TWICE_GPU3D_RE_H
#define TWICE_GPU3D_RE_H

#include "common/types.h"
#include "libtwice/nds/defs.h"
#include "nds/gpu/3d/gpu3d_types.h"
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
	bool last_poly_is_shadow_mask{};
	u32 outside_opaque_id_attr{};
	s32 outside_depth{};

	std::array<std::array<color4, 256>, 192> color_buf[2]{};
	std::array<std::array<s32, 256>, 192> depth_buf[2]{};
	/*
	 * attributes
	 * 0		edge
	 * 1		opaque
	 * 7		backface
	 * 8-12		antialising coverage
	 * 11		translucent set new depth
	 * 14		depth test equal mode
	 * 15		fog
	 * 16-21	translucent poly id
	 * 24-29	opaque poly id
	 */
	std::array<std::array<u32, 256>, 192> attr_buf[2]{};
	std::array<std::array<u8, 256>, 192> stencil_buf{};

	bool enabled{};
	vertex_ram *vtx_ram{};
	polygon_ram *poly_ram{};
	std::array<re_polygon, 2048> polys{};
	gpu_3d_engine *gpu{};
};

void re_render_frame(rendering_engine *re);

} // namespace twice

#endif
