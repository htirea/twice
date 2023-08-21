#ifndef TWICE_COLOR_H
#define TWICE_COLOR_H

#include "common/types.h"

namespace twice {

inline u32
bgr555_to_bgr888(u16 color)
{
	u32 r = color & 0x1F;
	u32 g = color >> 5 & 0x1F;
	u32 b = color >> 10 & 0x1F;

	r = (r * 527 + 23) >> 6;
	g = (g * 527 + 23) >> 6;
	b = (b * 527 + 23) >> 6;

	return b << 16 | g << 8 | r;
}

} // namespace twice

#endif
