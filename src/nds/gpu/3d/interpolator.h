#ifndef TWICE_GPU3D_INTERPOLATOR_H
#define TWICE_GPU3D_INTERPOLATOR_H

#include "common/types.h"

namespace twice {

struct interpolator {
	s32 x0;
	s32 x1;
	s32 x;
	s32 w0;
	s32 w1;
	s32 w0_n;
	s32 w0_d;
	s32 w1_d;
	s64 denom;
	u32 pfactor;
	u32 pfactor_r;
	int precision;
};

} // namespace twice

#endif
