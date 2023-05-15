#ifndef TWICE_GPU_H
#define TWICE_GPU_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct NDS;

struct Gpu {
	u16 ly{};
};

void gpu_on_hblank_start(NDS *nds);
void gpu_on_hblank_end(NDS *nds);

} // namespace twice

#endif
