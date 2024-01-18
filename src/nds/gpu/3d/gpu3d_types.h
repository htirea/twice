#ifndef TWICE_GPU3D_TYPES_H
#define TWICE_GPU3D_TYPES_H

#include "common/types.h"

namespace twice {

struct rendering_engine;
struct geometry_engine;
struct gpu_3d_engine;

struct vertex {
	std::array<s32, 4> pos{};  /* x, y, z, w */
	std::array<s32, 5> attr{}; /* r, g, b, s, t */
	s32 sx{};
	s32 sy{};
	u32 reused : 2 {};
	u32 strip_vtx : 2 {};
};

struct polygon {
	u32 num_vtxs{};
	std::array<vertex *, 10> vtxs{};
	std::array<s32, 10> w{};
	std::array<s32, 10> z{};
	u32 attr{};
	u32 tx_param{};
	u32 pltt_base{};
	int wshift{};
	bool wbuffering{};
	bool translucent{};
	bool backface{};
	u32 start_vtx{};
	u32 end_vtx{};
	std::pair<int, s32> sortkey{};
};

struct interpolator {
	struct attribute {
		s32 y{};
		s32 y_rem{};
		s32 y0{};
		s32 y1{};
		s32 y_len{};
		s32 y_step{};
		s32 y_rem_step{};
		bool positive{};
	};

	s32 x0{};
	s32 x1{};
	s32 x{};
	s32 x_len{};
	s32 w0{};
	s32 w1{};
	s32 w0_n{};
	s32 w0_d{};
	s32 w1_d{};
	s64 numer{};
	s64 denom{};
	u32 pfactor{};
	u32 pfactor_r{};
	int precision{};
	bool wbuffering{};
	s32 next_z_x{};
	s32 next_attr_x{};
	/* r, g, b, s, t, w, z */
	attribute attrs[7]{};
};

struct slope {
	s32 x0{};
	s32 y0{};
	s32 m{};
	s32 dx{};
	s32 dy{};
	bool negative{};
	bool xmajor{};
	bool wide{};
	bool vertical{};
	bool left{};
	bool line{};
	vertex *v0{};
	vertex *v1{};
	interpolator i{};
};

struct re_polygon {
	polygon *p{};
	u32 l{};
	u32 prev_l{};
	u32 r{};
	u32 prev_r{};
	slope sl{};
	slope sr{};
	s32 start_y{};
	s32 top_edge_y{};
	s32 bottom_edge_y{};
	s32 end_y{};
	u32 id{};
	bool shadow{};
	bool horizontal_line{};
};

struct vertex_ram {
	std::array<vertex, 6144> vtxs;
	u32 count{};
};

struct polygon_ram {
	std::array<polygon, 2048> polys;
	u32 count{};
};

} // namespace twice

#endif
