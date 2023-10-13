#include "nds/mem/vram.h"
#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

using vertex = gpu_3d_engine::vertex;
using polygon = gpu_3d_engine::polygon;
using polygon_info = gpu_3d_engine::rendering_engine::polygon_info;
using slope = gpu_3d_engine::rendering_engine::slope;
using interpolator = gpu_3d_engine::rendering_engine::interpolator;

void clear_stencil_buffer(gpu_3d_engine *gpu);

static void
interp_setup(interpolator *i, s32 x0, s32 x1, s32 w0, s32 w1, bool edge)
{
	i->x0 = x0;
	i->x1 = x1;
	i->w0 = w0;
	i->w1 = w1;
	i->denom = (s64)w0 * w1 * (x1 - x0);

	if ((edge && w0 == w1 && !(w0 & 0x7E)) ||
			(!edge && w0 == w1 && !(w0 & 0x7F))) {
		i->precision = 0;
		return;
	}

	i->w0_n = w0;
	i->w0_d = w0;
	i->w1_d = w1;

	if (edge) {
		if ((w0 & 1) && !(w1 & 1)) {
			i->w0_n--;
			i->w0_d++;
		} else {
			i->w0_n &= 0xFFFE;
			i->w0_d &= 0xFFFE;
			i->w1_d &= 0xFFFE;
		}
		i->precision = 9;
	} else {
		i->precision = 8;
	}
}

static u32
interp_get_yfactor(interpolator *i)
{
	s64 numer = ((s64)i->w0_n << i->precision) * ((s64)i->x - i->x0);
	s64 denom = (i->w1_d * ((s64)i->x1 - i->x) +
			i->w0_d * ((s64)i->x - i->x0));

	return denom == 0 ? 0 : numer / denom;
}

static void
interp_update_yfactor(interpolator *i)
{
	i->yfactor = interp_get_yfactor(i);
}

static void
interp_update_x(interpolator *i, s32 x)
{
	i->x = x;
	interp_update_yfactor(i);
}

static s32
interpolate_linear(interpolator *i, s32 y0, s32 y1)
{
	s64 numer = y0 * ((s64)i->x1 - i->x) + y1 * ((s64)i->x - i->x0);
	s64 denom = (s64)i->x1 - i->x0;

	return denom == 0 ? y0 : numer / denom;
}

static void
interpolate_linear_multiple(interpolator *i, s32 *y0, s32 *y1, s32 *y)
{
	s64 denom = (s64)i->x1 - i->x0;
	s64 f0 = (s64)i->x1 - i->x;
	s64 f1 = (s64)i->x - i->x0;

	if (denom == 0) {
		for (u32 j = 0; j < 5; j++) {
			y[j] = y0[j];
		}
	} else {
		for (u32 j = 0; j < 5; j++) {
			y[j] = (y0[j] * f0 + y1[j] * f1) / denom;
		}
	}
}

static s32
interpolate_perspective(interpolator *i, s32 y0, s32 y1)
{
	if (y0 <= y1) {
		return y0 + ((y1 - y0) * i->yfactor >> i->precision);
	} else {
		u32 one = (u32)1 << i->precision;
		return y1 + ((y0 - y1) * (one - i->yfactor) >> i->precision);
	}
}

static void
interpolate_perspective_multiple(interpolator *i, s32 *y0, s32 *y1, s32 *y)
{
	u32 factor = i->yfactor;
	u32 factor_r = ((u32)1 << i->precision) - i->yfactor;

	for (u32 j = 0; j < 5; j++) {
		if (y0[j] <= y1[j]) {
			y[j] = y0[j] +
			       ((y1[j] - y0[j]) * factor >> i->precision);
		} else {
			y[j] = y1[j] +
			       ((y0[j] - y1[j]) * factor_r >> i->precision);
		}
	}
}

static s32
interpolate(interpolator *i, s32 y0, s32 y1)
{
	if (i->precision == 0) {
		return interpolate_linear(i, y0, y1);
	} else {
		return interpolate_perspective(i, y0, y1);
	}
}

static void
interpolate_multiple(interpolator *i, s32 *y0, s32 *y1, s32 *y)
{
	if (i->precision == 0) {
		interpolate_linear_multiple(i, y0, y1, y);
	} else {
		interpolate_perspective_multiple(i, y0, y1, y);
	}
}

static s32
interpolate_z(interpolator *i, s32 z0, s32 z1, bool wbuffering)
{
	if (wbuffering) {
		return interpolate(i, z0, z1);
	} else {
		return interpolate_linear(i, z0, z1);
	}
}

static void
setup_polygon_slope(
		slope *s, vertex *v0, vertex *v1, s32 w0, s32 w1, bool left)
{
	s32 x0 = v0->sx;
	s32 y0 = v0->sy;
	s32 x1 = v1->sx;
	s32 y1 = v1->sy;

	s->x0 = x0 << 18;
	s->y0 = y0;
	s->v0 = v0;
	s->v1 = v1;

	s->negative = x0 > x1;
	if (s->negative) {
		s->x0--;
		std::swap(x0, x1);
	}

	s32 dx = x1 - x0;
	s32 dy = y1 - y0;
	s->xmajor = dx > dy;

	if (s->xmajor || dx == dy) {
		s32 half = (s32)1 << 17;
		s->x0 = s->negative ? s->x0 - half : s->x0 + half;
	}

	s->m = dx;
	if (dy != 0) {
		s->m *= ((s32)1 << 18) / dy;
	} else {
		s->m = ((s32)1 << 18);
	}
	s->vertical = s->m == 0;

	bool adjust = (s->xmajor || dx == dy) && (left == s->negative);
	interp_setup(&s->interp, v0->sy - adjust, v1->sy - adjust, w0, w1,
			true);
}

static void
get_slope_x(slope *s, s32 *x, s32 scanline)
{
	s32 one = (s32)1 << 18;

	s32 dy = scanline - s->y0;
	if (s->xmajor && !s->negative) {
		x[0] = s->x0 + dy * s->m;
		x[1] = (x[0] & ~0x1FF) + s->m - one;
	} else if (s->xmajor) {
		x[1] = s->x0 - dy * s->m;
		x[0] = (x[1] | 0x1FF) - s->m + one;
	} else if (!s->negative) {
		x[0] = s->x0 + dy * s->m;
		x[1] = x[0];
	} else {
		x[0] = s->x0 - dy * s->m;
		x[1] = x[0];
	}

	x[0] >>= 18;
	x[1] >>= 18;
	x[1]++;
}

static void
get_slope_x_start_end(
		slope *sl, slope *sr, s32 *xstart, s32 *xend, s32 scanline)
{
	get_slope_x(sl, xstart, scanline);
	get_slope_x(sr, xend, scanline);
}

static void
setup_polygon_initial_slope(polygon *p, polygon_info *pi)
{
	u32 next_l, next_r;
	if (p->vertices[p->start]->sy == p->vertices[p->end]->sy) {
		next_l = p->start;
		next_r = p->end;
	} else {
		next_l = (p->start + 1) % p->num_vertices;
		next_r = (p->start - 1 + p->num_vertices) % p->num_vertices;
	}

	if (p->backface) {
		std::swap(next_l, next_r);
	}

	setup_polygon_slope(&pi->left_slope, p->vertices[p->start],
			p->vertices[next_l], p->normalized_w[p->start],
			p->normalized_w[next_l], true);
	pi->prev_left = p->start;
	pi->left = next_l;

	setup_polygon_slope(&pi->right_slope, p->vertices[p->start],
			p->vertices[next_r], p->normalized_w[p->start],
			p->normalized_w[next_r], false);
	pi->prev_right = p->start;
	pi->right = next_r;
}

static void
setup_polygon_scanline(gpu_3d_engine *gpu, s32 scanline, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pi = &gpu->re.poly_info[poly_num];

	u32 curr, next;

	if (p->vertices[pi->left]->sy == scanline) {
		curr = pi->left;
		next = pi->left;
		while (next != p->end && p->vertices[next]->sy <= scanline) {
			curr = next;
			if (p->backface) {
				next = (next - 1 + p->num_vertices) %
				       p->num_vertices;
			} else {
				next = (next + 1) % p->num_vertices;
			}
		}
		if (next != curr) {
			setup_polygon_slope(&pi->left_slope, p->vertices[curr],
					p->vertices[next],
					p->normalized_w[curr],
					p->normalized_w[next], true);
			pi->prev_left = curr;
			pi->left = next;
		}
	}
	if (p->vertices[pi->right]->sy == scanline) {
		curr = pi->right;
		next = pi->right;
		while (next != p->end && p->vertices[next]->sy <= scanline) {
			curr = next;
			if (p->backface) {
				next = (next + 1) % p->num_vertices;
			} else {
				next = (next - 1 + p->num_vertices) %
				       p->num_vertices;
			}
		}
		if (next != curr) {
			setup_polygon_slope(&pi->right_slope,
					p->vertices[curr], p->vertices[next],
					p->normalized_w[curr],
					p->normalized_w[next], false);
			pi->prev_right = curr;
			pi->right = next;
		}
	}
}

static s32
clamp_or_repeat_texcoords(s32 s, s32 size, bool clamp, bool flip)
{
	if (s < 0 || s > size - 1) {
		if (clamp) {
			s = std::clamp(s, 0, size - 1);
		} else {
			flip = flip && s & size;
			s &= size - 1;
			if (flip) {
				s = size - 1 - s;
			}
		}
	}

	return s;
}

static void
blend_bgr555_11_3d(u16 color0, u16 color1, u8 *color_out)
{
	u8 r0 = color0 & 0x1F;
	u8 g0 = color0 >> 5 & 0x1F;
	u8 b0 = color0 >> 10 & 0x1F;
	u8 r1 = color1 & 0x1F;
	u8 g1 = color1 >> 5 & 0x1F;
	u8 b1 = color1 >> 10 & 0x1F;

	color_out[0] = r0 + r1;
	color_out[1] = g0 + g1;
	color_out[2] = b0 + b1;
}

static void
blend_bgr555_53_3d(u16 color0, u16 color1, u8 *color_out)
{
	u8 r0 = color0 & 0x1F;
	u8 g0 = color0 >> 5 & 0x1F;
	u8 b0 = color0 >> 10 & 0x1F;
	u8 r1 = color1 & 0x1F;
	u8 g1 = color1 >> 5 & 0x1F;
	u8 b1 = color1 >> 10 & 0x1F;

	color_out[0] = (r0 * 5 + r1 * 3) >> 2;
	color_out[1] = (g0 * 5 + g1 * 3) >> 2;
	color_out[2] = (b0 * 5 + b1 * 3) >> 2;
}

static void
unpack_abgr1555_3d(u16 color, u8 *color_out)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;
	u8 a = color >> 15 & 1;

	if (r != 0)
		r = (r << 1) + 1;
	if (g != 0)
		g = (g << 1) + 1;
	if (b != 0)
		b = (b << 1) + 1;
	if (a != 0)
		a = 31;

	color_out[0] = r;
	color_out[1] = g;
	color_out[2] = b;
	color_out[3] = a;
}

static color4
get_texture_color(gpu_3d_engine *gpu, polygon *p, s32 s, s32 t)
{
	u8 tx_color[4]{};

	u32 tx_format = p->tx_param >> 26 & 7;
	bool color_0_transparent = p->tx_param & BIT(29);
	bool s_clamp = !(p->tx_param & BIT(16));
	bool t_clamp = !(p->tx_param & BIT(17));
	bool s_flip = p->tx_param & BIT(18);
	bool t_flip = p->tx_param & BIT(19);
	s32 s_size = 8 << (p->tx_param >> 20 & 7) << 4;
	s32 t_size = 8 << (p->tx_param >> 23 & 7) << 4;
	s = clamp_or_repeat_texcoords(s, s_size, s_clamp, s_flip);
	t = clamp_or_repeat_texcoords(t, t_size, t_clamp, t_flip);

	s_size >>= 4;
	t_size >>= 4;

	u32 base_offset = (p->tx_param & 0xFFFF) << 3;

	switch (tx_format) {
	case 2:
	{
		u32 offset = base_offset;
		offset += (t >> 4) * (s_size >> 2);
		offset += (s >> 4) >> 2;
		u8 color_num = vram_read_texture<u8>(gpu->nds, offset);
		color_num = color_num >> ((s >> 4 & 3) << 1) & 3;
		if (color_num != 0 || !color_0_transparent) {
			u32 palette_offset =
					(p->pltt_base << 3) + (color_num << 1);
			u16 color = vram_read_texture_palette<u16>(
					gpu->nds, palette_offset);
			unpack_bgr555_3d(color, tx_color);
			tx_color[3] = 31;
		}
		break;
	}
	case 3:
	{
		u32 offset = base_offset;
		offset += (t >> 4) * (s_size >> 1);
		offset += (s >> 4) >> 1;
		u8 color_num = vram_read_texture<u8>(gpu->nds, offset);
		color_num = color_num >> ((s >> 4 & 1) << 2) & 0xF;
		if (color_num != 0 || !color_0_transparent) {
			u32 palette_offset =
					(p->pltt_base << 4) + (color_num << 1);
			u16 color = vram_read_texture_palette<u16>(
					gpu->nds, palette_offset);
			unpack_bgr555_3d(color, tx_color);
			tx_color[3] = 31;
		}
		break;
	}
	case 4:
	{
		u32 offset = base_offset + (t >> 4) * s_size + (s >> 4);
		u8 color_num = vram_read_texture<u8>(gpu->nds, offset);
		if (color_num != 0 || !color_0_transparent) {
			u32 palette_offset =
					(p->pltt_base << 4) + (color_num << 1);
			u16 color = vram_read_texture_palette<u16>(
					gpu->nds, palette_offset);
			unpack_bgr555_3d(color, tx_color);
			tx_color[3] = 31;
		}
		break;
	}
	case 1:
	{
		u32 offset = base_offset + (t >> 4) * s_size + (s >> 4);
		u8 data = vram_read_texture<u8>(gpu->nds, offset);
		u8 color_num = data & 0x1F;
		u8 alpha = data >> 5;
		u32 palette_offset = (p->pltt_base << 4) + (color_num << 1);
		u16 color = vram_read_texture_palette<u16>(
				gpu->nds, palette_offset);
		unpack_bgr555_3d(color, tx_color);
		tx_color[3] = (alpha << 2) + (alpha >> 1);
		break;
	}
	case 6:
	{
		u32 offset = base_offset + (t >> 4) * s_size + (s >> 4);
		u8 data = vram_read_texture<u8>(gpu->nds, offset);
		u8 color_num = data & 7;
		u32 palette_offset = (p->pltt_base << 4) + (color_num << 1);
		u16 color = vram_read_texture_palette<u16>(
				gpu->nds, palette_offset);
		unpack_bgr555_3d(color, tx_color);
		tx_color[3] = data >> 3;
		break;
	}
	case 7:
	{
		u32 offset = base_offset;
		offset += (t >> 4) * s_size << 1;
		offset += (s >> 4) << 1;
		u16 color = vram_read_texture<u16>(gpu->nds, offset);
		unpack_abgr1555_3d(color, tx_color);
		break;
	}
	case 5:
	{
		u32 offset = base_offset;
		offset += (t >> 4 & ~3) * (s_size >> 2);
		offset += (s >> 4 & ~3);
		u32 data = vram_read_texture<u32>(gpu->nds, offset);
		u32 color_num = data >> ((t >> 4 & 3) << 3) >>
		                ((s >> 4 & 3) << 1);
		color_num &= 3;
		u32 index_offset = 0x20000 + ((offset & 0x1FFFF) >> 1);
		if (offset >= 0x40000) {
			index_offset += 0x10000;
		}
		u16 index_data =
				vram_read_texture<u32>(gpu->nds, index_offset);
		u32 palette_base = (p->pltt_base << 4) +
		                   ((index_data & 0x3FFF) << 2);
		u32 palette_mode = index_data >> 14;

		switch (color_num) {
		case 0:
		{
			u16 color = vram_read_texture_palette<u16>(
					gpu->nds, palette_base);
			unpack_bgr555_3d(color, tx_color);
			tx_color[3] = 31;
			break;
		}
		case 1:
		{
			u16 color = vram_read_texture_palette<u16>(
					gpu->nds, palette_base + 2);
			unpack_bgr555_3d(color, tx_color);
			tx_color[3] = 31;
			break;
		}
		case 2:
		{
			switch (palette_mode) {
			case 0:
			case 2:
			{
				u16 color = vram_read_texture_palette<u16>(
						gpu->nds, palette_base + 4);
				unpack_bgr555_3d(color, tx_color);
				tx_color[3] = 31;
				break;
			}
			case 1:
			case 3:
			{
				u16 color0 = vram_read_texture_palette<u16>(
						gpu->nds, palette_base);
				u16 color1 = vram_read_texture_palette<u16>(
						gpu->nds, palette_base + 2);
				if (palette_mode == 1) {
					blend_bgr555_11_3d(color0, color1,
							tx_color);
				} else {
					blend_bgr555_53_3d(color0, color1,
							tx_color);
				}
				tx_color[3] = 31;
			}
			}
			break;
		}
		case 3:
		{
			switch (palette_mode) {
			case 0:
			case 1:
				tx_color[3] = 0;
				break;
			case 2:
			{
				u16 color = vram_read_texture_palette<u16>(
						gpu->nds, palette_base + 6);
				unpack_bgr555_3d(color, tx_color);
				tx_color[3] = 31;
				break;
			}
			case 3:

			{
				u16 color0 = vram_read_texture_palette<u16>(
						gpu->nds, palette_base);
				u16 color1 = vram_read_texture_palette<u16>(
						gpu->nds, palette_base + 2);
				blend_bgr555_53_3d(color1, color0, tx_color);
				tx_color[3] = 31;
			}
			}
		}
		}
		break;
	}
	}

	return { tx_color[0], tx_color[1], tx_color[2], tx_color[3] };
}

static void
unpack_bgr555_3d(u16 color, u8 *r_out, u8 *g_out, u8 *b_out)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;

	if (r != 0)
		r = (r << 1) + 1;
	if (g != 0)
		g = (g << 1) + 1;
	if (b != 0)
		b = (b << 1) + 1;

	*r_out = r;
	*g_out = g;
	*b_out = b;
}

static color4
get_pixel_color(gpu_3d_engine *gpu, polygon *p, u8 rv, u8 gv, u8 bv, u8 av,
		s32 s, s32 t)
{
	u32 tx_format = p->tx_param >> 26 & 7;
	u32 blend_mode = p->attr >> 4 & 3;

	u8 rs, gs, bs;
	bool highlight_shading = blend_mode == 2 && gpu->re.r.disp3dcnt & 2;
	if (blend_mode == 2) {
		u16 toon_color = readarr<u16>(gpu->re.r.toon_table, rv & 0x3E);
		unpack_bgr555_3d(toon_color, &rs, &gs, &bs);

		if (highlight_shading) {
			gv = rv;
			bv = rv;
		} else {
			rv = rs;
			gv = gs;
			bv = bs;
		}
	}

	u8 r, g, b, a;
	if (tx_format == 0 || !(gpu->re.r.disp3dcnt & 1)) {
		r = rv;
		g = gv;
		b = bv;
		a = av;
	} else {
		color4 tx_color = get_texture_color(gpu, p, s, t);
		u8 rt = tx_color.r;
		u8 gt = tx_color.g;
		u8 bt = tx_color.b;
		u8 at = tx_color.a;

		if (blend_mode & 1) {
			switch (at) {
			case 0:
				r = rv;
				g = gv;
				b = bv;
				a = av;
				break;
			case 31:
				r = rt;
				g = gt;
				b = bt;
				a = av;
				break;
			default:
				r = (rt * at + rv * (31 - at)) >> 5;
				g = (gt * at + gv * (31 - at)) >> 5;
				b = (bt * at + bv * (31 - at)) >> 5;
				a = av;
			}
		} else {
			r = ((rt + 1) * (rv + 1) - 1) >> 6;
			g = ((gt + 1) * (gv + 1) - 1) >> 6;
			b = ((bt + 1) * (bv + 1) - 1) >> 6;
			a = ((at + 1) * (av + 1) - 1) >> 5;
		}
	}

	if (highlight_shading) {
		r = std::min(63, r + rs);
		g = std::min(63, g + gs);
		b = std::min(63, b + bs);
	}

	return { r, g, b, a };
}

static color4
alpha_blend(color4 s, color4 b)
{
	return {
		(u8)(((s.a + 1) * s.r + (31 - s.a) * b.r) >> 5),
		(u8)(((s.a + 1) * s.g + (31 - s.a) * b.g) >> 5),
		(u8)(((s.a + 1) * s.b + (31 - s.a) * b.b) >> 5),
		std::max(s.a, b.a),
	};
}

struct render_polygon_ctx {
	polygon *p;
	polygon_info *pi;
	s32 attr_l[5];
	s32 attr_r[5];
	s32 zl;
	s32 zr;
	s32 alpha;
	bool alpha_blending;
	u8 alpha_test_ref;
};

struct render_vertex_attrs {
	s32 z;
	s32 color[4];
	s32 tx[2];
};

static bool
depth_test_le_margin(render_polygon_ctx *ctx, s32 z, s32 z_dest)
{
	s32 margin = ctx->p->wbuffering ? 0xFF : 0x200;
	bool equal = z_dest - margin <= z && z <= z_dest + margin;

	return equal || z < z_dest;
}

static bool
depth_test_lt(render_polygon_ctx *, s32 z, s32 z_dest)
{
	return z < z_dest;
}

static bool
depth_test_le(render_polygon_ctx *, s32 z, s32 z_dest)
{
	return z <= z_dest;
}

static bool
depth_test_passed(gpu_3d_engine *gpu, render_polygon_ctx *ctx, s32 z, s32 x,
		s32 y)
{
	s32 z_dest = gpu->depth_buf[y][x];
	auto attr_dest = gpu->attr_buf[y][x];

	if (ctx->p->attr & BIT(14)) {
		return depth_test_le_margin(ctx, z, z_dest);
	}

	/* TODO: wireframe edge */

	if (!ctx->p->backface &&
			(!attr_dest.translucent && attr_dest.backface)) {
		return depth_test_le(ctx, z, z_dest);
	}

	return depth_test_lt(ctx, z, z_dest);
}

static void
render_opaque_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx, color4 color,
		s32 x, s32 y, s32 z)
{
	gpu->color_buf[y][x] = color;
	gpu->depth_buf[y][x] = z;

	auto& attr = gpu->attr_buf[y][x];
	attr.fog = ctx->p->attr >> 15 & 1;
	attr.translucent = 0;
	attr.backface = ctx->p->backface;
	attr.opaque_id = ctx->p->attr >> 24 & 0x3F;
}

static void
render_translucent_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		color4 color, s32 x, s32 y, s32 z)
{
	u32 poly_id = ctx->p->attr >> 24 & 0x3F;
	auto& attr = gpu->attr_buf[y][x];
	if (attr.translucent && attr.translucent_id == poly_id)
		return;

	if (!ctx->alpha_blending || gpu->color_buf[y][x].a == 0) {
		gpu->color_buf[y][x] = color;
	} else {
		gpu->color_buf[y][x] =
				alpha_blend(color, gpu->color_buf[y][x]);
	}

	if (ctx->p->attr & BIT(11)) {
		gpu->depth_buf[y][x] = z;
	}

	attr.fog = ctx->p->attr >> 15 & 1;
	attr.translucent_id = poly_id;
	attr.translucent = 1;
	attr.backface = ctx->p->backface;
}

static void
render_polygon_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		interpolator *span, s32 x, s32 y)
{
	bool shadow = ctx->pi->shadow;

	if (shadow) {
		if (!gpu->stencil_buf[x])
			return;
		gpu->stencil_buf[x] = false;
	}

	interp_update_x(span, x);

	s32 z = interpolate_z(span, ctx->zl, ctx->zr, ctx->p->wbuffering);
	if (!depth_test_passed(gpu, ctx, z, x, y))
		return;

	if (shadow) {
		auto& attr = gpu->attr_buf[y][x];
		u32 dest_id = attr.translucent ? attr.translucent_id
		                               : attr.opaque_id;
		if (ctx->pi->poly_id == dest_id)
			return;
	}

	s32 attr_result[5];
	interpolate_multiple(span, ctx->attr_l, ctx->attr_r, attr_result);
	u8 av = ctx->alpha == 0 ? 31 : ctx->alpha;

	color4 px_color = get_pixel_color(gpu, ctx->p, attr_result[0],
			attr_result[1], attr_result[2], av, attr_result[3],
			attr_result[4]);
	if (px_color.a <= ctx->alpha_test_ref) {
		return;
	}

	if (px_color.a == 31) {
		render_opaque_pixel(gpu, ctx, px_color, x, y, z);
	} else if (px_color.a != 0) {
		render_translucent_pixel(gpu, ctx, px_color, x, y, z);
	}
}

static void
render_shadow_mask_polygon_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		interpolator *span, s32 x, s32 y)
{
	interp_update_x(span, x);
	s32 z = interpolate_z(span, ctx->zl, ctx->zr, ctx->p->wbuffering);

	u8 av = ctx->alpha == 0 ? 31 : ctx->alpha;
	if (av < ctx->alpha_test_ref) {
		return;
	}

	if (!depth_test_passed(gpu, ctx, z, x, y)) {
		gpu->stencil_buf[x] = true;
	}
}

static void
render_polygon_scanline(gpu_3d_engine *gpu, s32 scanline, u32 poly_num,
		bool shadow_mask)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pi = &gpu->re.poly_info[poly_num];
	render_polygon_ctx ctx;
	ctx.p = p;
	ctx.pi = pi;

	if (shadow_mask && !gpu->re.last_poly_is_shadow_mask) {
		clear_stencil_buffer(gpu);
	}

	slope *sl = &pi->left_slope;
	slope *sr = &pi->right_slope;

	s32 xstart[2];
	s32 xend[2];
	get_slope_x_start_end(sl, sr, xstart, xend, scanline);

	if (xstart[0] > xend[0] || xstart[1] > xend[1]) {
		std::swap(sl, sr);
		std::swap(xstart[0], xend[0]);
		std::swap(xstart[1], xend[1]);
	}

	xend[0] = std::max(xend[0], xstart[1]);

	interp_update_x(&sl->interp, scanline);
	interp_update_x(&sr->interp, scanline);
	s32 wl = interpolate(&sl->interp, sl->interp.w0, sl->interp.w1);
	s32 wr = interpolate(&sr->interp, sr->interp.w0, sr->interp.w1);
	ctx.zl = interpolate_z(&sl->interp, p->z[pi->prev_left],
			p->z[pi->left], p->wbuffering);
	ctx.zr = interpolate_z(&sr->interp, p->z[pi->prev_right],
			p->z[pi->right], p->wbuffering);

	if (!shadow_mask) {
		interpolate_multiple(&sl->interp, sl->v0->attr, sl->v1->attr,
				ctx.attr_l);
		interpolate_multiple(&sr->interp, sr->v0->attr, sr->v1->attr,
				ctx.attr_r);
	}

	interpolator span;
	interp_setup(&span, xstart[0], xend[1] - 1 + !sr->vertical, wl, wr,
			false);

	ctx.alpha = p->attr >> 16 & 0x1F;

	bool wireframe_or_translucent = ctx.alpha != 31;
	bool antialiasing = gpu->re.r.disp3dcnt & BIT(4);
	bool edge_marking = gpu->re.r.disp3dcnt & BIT(5);
	bool last_scanline = scanline == 191;
	bool force_fill_edge = wireframe_or_translucent || antialiasing ||
	                       edge_marking || last_scanline;

	ctx.alpha_blending = gpu->re.r.disp3dcnt & BIT(3);
	if (!(gpu->re.r.disp3dcnt & BIT(2))) {
		ctx.alpha_test_ref = 0;
	} else {
		ctx.alpha_test_ref = gpu->re.r.alpha_test_ref;
	}

	bool fill_left = sl->negative || !sl->xmajor || force_fill_edge;
	if (fill_left) {
		s32 start = std::max(0, xstart[0]);
		s32 end = std::min(256, xstart[1]);

		for (s32 x = start; shadow_mask && x < end; x++) {
			render_shadow_mask_polygon_pixel(
					gpu, &ctx, &span, x, scanline);
		}

		for (s32 x = start; !shadow_mask && x < end; x++) {
			render_polygon_pixel(gpu, &ctx, &span, x, scanline);
		}
	}

	bool fill_middle = ctx.alpha != 0;
	if (fill_middle) {
		s32 start = std::max(0, xstart[1]);
		s32 end = std::min(256, xend[0]);

		for (s32 x = start; shadow_mask && x < end; x++) {
			render_shadow_mask_polygon_pixel(
					gpu, &ctx, &span, x, scanline);
		}

		for (s32 x = start; !shadow_mask && x < end; x++) {
			render_polygon_pixel(gpu, &ctx, &span, x, scanline);
		}
	}

	bool fill_right = (!sr->negative && sr->xmajor) || sr->vertical ||
	                  force_fill_edge;
	if (fill_right) {
		s32 start = std::max(0, xend[0] - sr->vertical);
		s32 end = std::min(256, xend[1] - sr->vertical);

		for (s32 x = start; shadow_mask && x < end; x++) {
			render_shadow_mask_polygon_pixel(
					gpu, &ctx, &span, x, scanline);
		}

		for (s32 x = start; !shadow_mask && x < end; x++) {
			render_polygon_pixel(gpu, &ctx, &span, x, scanline);
		}
	}
}

static bool
polygon_not_in_scanline(polygon_info *pi, s32 y)
{
	return y < pi->y_start || y >= pi->y_end;
}

static void
render_3d_scanline(gpu_3d_engine *gpu, u32 scanline)
{
	auto& re = gpu->re;

	for (u32 i = 0; i < re.num_polygons; i++) {
		polygon_info *pi = &re.poly_info[i];
		if (polygon_not_in_scanline(pi, scanline))
			continue;
		setup_polygon_scanline(gpu, scanline, i);
		bool shadow_mask = pi->shadow && pi->poly_id == 0;
		render_polygon_scanline(gpu, scanline, i, shadow_mask);
		re.last_poly_is_shadow_mask = shadow_mask;
	}
}

static color4
axbgr_51555_to_abgr5666(u32 in)
{
	u8 r = in & 0x1F;
	u8 g = in >> 5 & 0x1F;
	u8 b = in >> 10 & 0x1F;
	u8 a = in >> 16 & 0x1F;

	if (r)
		r = (r << 1) + 1;
	if (g)
		g = (g << 1) + 1;
	if (b)
		b = (b << 1) + 1;

	return { r, g, b, a };
}

static void
clear_buffers(gpu_3d_engine *gpu)
{
	auto& re = gpu->re;

	if (!(re.r.disp3dcnt & BIT(14))) {
		color4 color = axbgr_51555_to_abgr5666(re.r.clear_color);
		u32 attr = re.r.clear_color & 0x3F008000;
		u32 fog = attr >> 15 & 3;
		u32 opaque_id = attr >> 24 & 0x3F;
		u32 depth = 0x200 * re.r.clear_depth + 0x1FF;

		for (u32 i = 0; i < 192; i++) {
			for (u32 j = 0; j < 256; j++) {
				gpu->color_buf[i][j] = color;
				gpu->depth_buf[i][j] = depth;
				gpu->attr_buf[i][j].fog = fog;
				gpu->attr_buf[i][j].translucent_id = 0;
				gpu->attr_buf[i][j].translucent = 0;
				gpu->attr_buf[i][j].backface = 0;
				gpu->attr_buf[i][j].opaque_id = opaque_id;
			}
		}

		clear_stencil_buffer(gpu);
	} else {
		/* TODO: */
		LOG("clear buffer bitmap\n");
	}
}

static void
find_polygon_start_end_sortkey(polygon *p, bool manual_sort)
{
	if (p->num_vertices <= 1) {
		p->start = 0;
		p->end = 0;
		return;
	}

	u32 start = 0;
	s32 start_x = p->vertices[0]->sx;
	s32 start_y = p->vertices[0]->sy;

	u32 end = 0;
	s32 end_x = p->vertices[0]->sx;
	s32 end_y = p->vertices[0]->sy;

	for (u32 i = 1; i < p->num_vertices; i++) {
		vertex *v = p->vertices[i];
		if (v->sy < start_y || (v->sy == start_y && v->sx < start_x)) {
			start = i;
			start_x = v->sx;
			start_y = v->sy;
		}
		if (v->sy > end_y || (v->sy == end_y && v->sx >= end_x)) {
			end = i;
			end_x = v->sx;
			end_y = v->sy;
		}
	}

	p->start = start;
	p->end = end;

	if (p->translucent) {
		p->sortkey = { 1, manual_sort ? 0 : start_y };
	} else {
		p->sortkey = { 0, start_y };
	}
}

static void
set_polygon_bounds(polygon *p, polygon_info *pi)
{
	s32 y_start = p->vertices[p->start]->sy;
	s32 y_end = p->vertices[p->end]->sy;

	if (y_start == y_end) {
		y_end++;
	}

	pi->y_start = y_start;
	pi->y_end = y_end;
}

static void
setup_polygon(gpu_3d_engine *gpu, u32 poly_num)
{
	auto& re = gpu->re;
	polygon *p = re.polygons[poly_num];
	polygon_info *pi = &re.poly_info[poly_num];

	set_polygon_bounds(p, pi);
	setup_polygon_initial_slope(p, pi);
	pi->poly_id = p->attr >> 24 & 0x3F;
	pi->shadow = (p->attr >> 4 & 3) == 3;
}

static bool
cmp_polygon(polygon *a, polygon *b)
{
	return a->sortkey < b->sortkey;
}

static void
setup_polygons(gpu_3d_engine *gpu)
{
	auto& re = gpu->re;
	auto& pr = gpu->poly_ram[gpu->re_buf];

	re.num_polygons = pr.count;

	for (u32 i = 0; i < pr.count; i++) {
		re.polygons[i] = &pr.polygons[i];
		find_polygon_start_end_sortkey(re.polygons[i], re.manual_sort);
	}
	std::stable_sort(re.polygons, re.polygons + pr.count, cmp_polygon);

	for (u32 i = 0; i < pr.count; i++) {
		setup_polygon(gpu, i);
	}
}

void
gpu3d_render_frame(gpu_3d_engine *gpu)
{
	clear_buffers(gpu);
	setup_polygons(gpu);

	for (u32 i = 0; i < 192; i++) {
		render_3d_scanline(gpu, i);
	}
}

void
clear_stencil_buffer(gpu_3d_engine *gpu)
{
	for (u32 i = 0; i < 256; i++) {
		gpu->stencil_buf[i] = 0;
	}
}

} // namespace twice
