#ifndef TWICE_GPU3D_GE_POLY_H
#define TWICE_GPU3D_GE_POLY_H

#include "common/types.h"
#include "nds/gpu/3d/gpu3d_types.h"

namespace twice {

struct geometry_engine;

void ge_add_vertex(geometry_engine *ge);
std::vector<vertex> ge_clip_polygon(std::span<vertex *>, u32 num_vertices,
		bool render_clipped_far);

} // namespace twice

#endif
