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
};

union color_u {
	u32 v;
	color4 p;

	color_u() : v(0) {}

	color_u(u32 v) : v(v) {}

	color_u(color4 p) : p(p) {}
};

inline void
unpack_bgr555_3d(u16 color, std::span<u8, 3> color_out)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;

	color_out[0] = (r << 1) + (r != 0);
	color_out[1] = (g << 1) + (g != 0);
	color_out[2] = (b << 1) + (b != 0);
}

inline void
unpack_abgr1555_3d(u16 color, u8 *r_out, u8 *g_out, u8 *b_out, u8 *a_out)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;
	u8 a = color >> 15 & 1;

	*r_out = (r << 1) + (r != 0);
	*g_out = (g << 1) + (g != 0);
	*b_out = (b << 1) + (b != 0);
	*a_out = a ? 31 : 0;
}

inline color4
unpack_bgr555_3d(u16 color)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;

	r = (r << 1) + (r != 0);
	g = (g << 1) + (g != 0);
	b = (b << 1) + (b != 0);

	return { r, g, b, 0x1F };
}

inline color4
unpack_abgr1555_3d(u16 color)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;
	u8 a = color >> 15 & 1;

	r = (r << 1) + (r != 0);
	g = (g << 1) + (g != 0);
	b = (b << 1) + (b != 0);
	a = a ? 31 : 0;

	return { r, g, b, a };
}

inline void
unpack_bgr555_3d(u16 color, u8 *r_out, u8 *g_out, u8 *b_out)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;

	*r_out = (r << 1) + (r != 0);
	*g_out = (g << 1) + (g != 0);
	*b_out = (b << 1) + (b != 0);
}

inline color4
unpack_bgr555_2d(u16 color)
{
	u8 r = (color & 0x1F) << 1;
	u8 g = (color >> 5 & 0x1F) << 1;
	u8 b = (color >> 10 & 0x1F) << 1;

	return { r, g, b, 0x1F };
}

inline u32
pack_to_bgr888(color4 color)
{
	u32 r = (color.r * 259 + 33) >> 6;
	u32 g = (color.g * 259 + 33) >> 6;
	u32 b = (color.b * 259 + 33) >> 6;

	return 0xFF000000 | b << 16 | g << 8 | r;
}

inline void
blend_bgr555_11_3d(u16 color0, u16 color1, u8 *r_out, u8 *g_out, u8 *b_out)
{
	u8 r0 = color0 & 0x1F;
	u8 g0 = color0 >> 5 & 0x1F;
	u8 b0 = color0 >> 10 & 0x1F;
	u8 r1 = color1 & 0x1F;
	u8 g1 = color1 >> 5 & 0x1F;
	u8 b1 = color1 >> 10 & 0x1F;

	*r_out = r0 + r1;
	*g_out = g0 + g1;
	*b_out = b0 + b1;
}

inline void
blend_bgr555_53_3d(u16 color0, u16 color1, u8 *r_out, u8 *g_out, u8 *b_out)
{
	u8 r0 = color0 & 0x1F;
	u8 g0 = color0 >> 5 & 0x1F;
	u8 b0 = color0 >> 10 & 0x1F;
	u8 r1 = color1 & 0x1F;
	u8 g1 = color1 >> 5 & 0x1F;
	u8 b1 = color1 >> 10 & 0x1F;

	*r_out = (r0 * 5 + r1 * 3) >> 2;
	*g_out = (g0 * 5 + g1 * 3) >> 2;
	*b_out = (b0 * 5 + b1 * 3) >> 2;
}

} // namespace twice

#endif
