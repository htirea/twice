#ifndef TWICE_GPU_3D_H
#define TWICE_GPU_3D_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct gpu_3d_engine {
	gpu_3d_engine(nds_ctx *nds);

	nds_ctx *nds{};
};

} // namespace twice

#endif
