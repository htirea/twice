#ifndef TWICE_GPU3D_TYPES_H
#define TWICE_GPU3D_TYPES_H

#include "common/types.h"

namespace twice {

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
