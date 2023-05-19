#ifndef TWICE_GPU_H
#define TWICE_GPU_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct NDS;

struct Gpu {
	Gpu(NDS *nds);

	NDS *nds{};

	void draw_current_scanline();

	void on_scanline_start();
};

} // namespace twice

#endif
