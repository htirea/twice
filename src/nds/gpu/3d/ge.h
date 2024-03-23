#ifndef TWICE_GPU3D_GE_H
#define TWICE_GPU3D_GE_H

#include "nds/gpu/3d/gpu3d_types.h"

namespace twice {

struct gpu_3d_engine;

struct geometry_engine {
	u32 cmd_params[32]{};

	u32 mtx_mode{};
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

	std::array<u8, 3> vtx_color{};
	std::array<u8, 3> diffuse_color{};
	std::array<u8, 3> ambient_color{};
	std::array<u8, 3> specular_color{};
	std::array<u8, 3> emission_color{};
	std::array<u8, 3> light_color[4]{};
	std::array<s32, 3> light_vec[4]{};
	std::array<s32, 3> half_vec[4]{};
	std::array<s32, 3> normal_vec{};
	std::array<s32, 2> texcoord{};
	std::array<s32, 2> vtx_texcoord{};

	s32 disp_1dot_depth{};
	u32 swap_bits{};
	u32 poly_attr{};
	u32 poly_attr_s{};
	u32 teximage_param{};
	u32 pltt_base{};
	s32 vx{};
	s32 vy{};
	s32 vz{};
	std::array<u8, 128> shiny_table{};
	bool shiny_table_enabled{};

	u8 primitive_type{};
	std::array<vertex *, 2> last_strip_vtx{};
	std::array<vertex, 4> vtx_buf{};
	u32 vtx_count{};

	std::array<u8, 2> viewport_x{};
	std::array<u8, 2> viewport_y{};
	u16 viewport_w{};
	u16 viewport_h{};

	std::array<s32, 4> pos_test_result{};
	std::array<s32, 3> vec_test_result{};

	bool enabled{};
	vertex_ram *vtx_ram{};
	polygon_ram *poly_ram{};
	gpu_3d_engine *gpu{};
};

void ge_execute_command(geometry_engine *ge, u8 cmd);

} // namespace twice

#endif
