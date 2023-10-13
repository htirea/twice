#ifndef TWICE_COLOR_H
#define TWICE_COLOR_H

#include "common/types.h"

namespace twice {

struct color4 {
	u8 r;
	u8 g;
	u8 b;
	u8 a;

	color4() : r(0), g(0), b(0), a(0) {}

	color4(u8 r, u8 g, u8 b, u8 a) : r(r), g(g), b(b), a(a) {}

	color4(u8 r, u8 g, u8 b) : r(r), g(g), b(b), a(0) {}
};

union color_u {
	u32 v;
	color4 p;

	color_u() : v(0) {}

	color_u(u32 v) : v(v) {}

	color_u(color4 p) : p(p) {}
};

inline void
unpack_bgr555_3d(u16 color, u8 *color_out)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;

	color_out[0] = (r << 1) + (r != 0);
	color_out[1] = (g << 1) + (g != 0);
	color_out[2] = (b << 1) + (b != 0);
}

} // namespace twice

#endif
