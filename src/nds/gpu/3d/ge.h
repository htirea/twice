#ifndef TWICE_GPU3D_GE_H
#define TWICE_GPU3D_GE_H

#include "nds/gpu/3d/polygon.h"

namespace twice {

struct gpu_3d_engine;

struct geometry_engine {
	std::array<u32, 32> cmd_params{};

	std::array<s32, 16> clip_mtx{};
	std::array<s32, 16> proj_mtx{};
	std::array<s32, 16> position_mtx{};
	std::array<s32, 16> vector_mtx{};
	std::array<s32, 16> texture_mtx{};
	std::array<s32, 16> proj_stack[1]{};
	std::array<s32, 16> position_stack[32]{};
	std::array<s32, 16> vector_stack[32]{};
	std::array<s32, 16> texture_stack[1]{};
	u32 proj_sp{};
	u32 position_sp{};
	u32 texture_sp{};

	s32 disp_1dot_depth{};
	u32 swap_bits{};

	vertex_ram *vtx_ram{};
	polygon_ram *poly_ram{};
	gpu_3d_engine *gpu{};
};

void ge_execute_command(geometry_engine *ge, u8 cmd);

} // namespace twice

#endif
