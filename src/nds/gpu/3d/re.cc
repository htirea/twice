#include "nds/mem/vram.h"
#include "nds/nds.h"

namespace twice {

using vertex = gpu_3d_engine::vertex;
using polygon = gpu_3d_engine::polygon;
using polygon_info = gpu_3d_engine::rendering_engine::polygon_info;
using slope = gpu_3d_engine::rendering_engine::slope;
using interpolator = gpu_3d_engine::rendering_engine::interpolator;

static void interp_setup(interpolator *i, s32 x0, s32 x1, s32 w0, s32 w1);

static u32
color6_to_abgr666(color6 color)
{
	return (u32)(0x1F) << 18 | (u32)color.b << 12 | (u32)color.g << 6 |
	       (u32)color.r;
}

static bool
polygon_not_in_scanline(gpu_3d_engine *gpu, s32 scanline, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pinfo = &gpu->re.poly_info[poly_num];

	s32 ystart = p->vertices[pinfo->start]->sy;
	s32 yend = p->vertices[pinfo->end]->sy;

	bool flat_line = p->vertices[pinfo->start]->sy ==
	                 p->vertices[pinfo->end]->sy;
	if (flat_line) {
		yend++;
	}

	return scanline < ystart || scanline >= yend;
}

static void
set_slope_interp_x(slope *s, s32 scanline, s32 x)
{
	if (s->xmajor) {
		s->interp.x = x;
	} else {
		s->interp.x = scanline;
	}
}

static void
setup_polygon_slope(slope *s, vertex *v0, vertex *v1, s32 w0, s32 w1)
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

	if (s->xmajor) {
		interp_setup(&s->interp, v0->sx, v1->sx, w0, w1);
	} else {
		interp_setup(&s->interp, v0->sy, v1->sy, w0, w1);
	}
}

static void
setup_polygon_slope_vertical(slope *s, vertex *v, s32 w0)
{
	vertex v1;
	v1.sx = v->sx;
	v1.sy = v->sy + 1;
	setup_polygon_slope(s, v, &v1, w0, w0);
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
interp_setup(interpolator *i, s32 x0, s32 x1, s32 w0, s32 w1)
{
	i->x0 = x0;
	i->x1 = x1;
	i->w0 = w0;
	i->w1 = w1;
	i->denom = (s64)w0 * w1 * (x1 - x0);
}

static void
interp_update_w(interpolator *i)
{
	s64 denom = (s64)i->w1 * (i->x1 - i->x) + (s64)i->w0 * (i->x - i->x0);

	if (denom == 0) {
		i->w = i->w0;
		return;
	}

	i->w = i->denom / denom;
}

static s32
interpolate(interpolator *i, s32 y0, s32 y1)
{
	s64 numer = (s64)i->w1 * y0 * (i->x1 - i->x) +
	            (s64)i->w0 * y1 * (i->x - i->x0);

	if (i->denom == 0) {
		return y0;
	}

	return i->w * numer / i->denom;
}

static void
setup_polygon_initial_slope(gpu_3d_engine *gpu, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pinfo = &gpu->re.poly_info[poly_num];

	u32 next_l = (pinfo->start + 1) % p->num_vertices;
	u32 next_r = (pinfo->start - 1 + p->num_vertices) % p->num_vertices;
	if (p->backface) {
		std::swap(next_l, next_r);
	}

	setup_polygon_slope(&pinfo->left_slope, p->vertices[pinfo->start],
			p->vertices[next_l], p->normalized_w[pinfo->start],
			p->normalized_w[next_l]);
	pinfo->prev_left = pinfo->start;
	pinfo->left = next_l;

	setup_polygon_slope(&pinfo->right_slope, p->vertices[pinfo->start],
			p->vertices[next_r], p->normalized_w[pinfo->start],
			p->normalized_w[next_r]);
	pinfo->prev_right = pinfo->start;
	pinfo->right = next_r;
}

static void
setup_polygon_scanline(gpu_3d_engine *gpu, s32 scanline, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pinfo = &gpu->re.poly_info[poly_num];

	if (polygon_not_in_scanline(gpu, scanline, poly_num))
		return;

	if (p->vertices[pinfo->start]->sy == p->vertices[pinfo->end]->sy) {
		setup_polygon_slope_vertical(&pinfo->left_slope,
				p->vertices[pinfo->start], 1);
		setup_polygon_slope_vertical(&pinfo->right_slope,
				p->vertices[pinfo->end], 1);
	} else {
		u32 curr, next;

		if (p->vertices[pinfo->left]->sy == scanline) {
			curr = pinfo->left;
			next = pinfo->left;
			while (next != pinfo->end &&
					p->vertices[next]->sy <= scanline) {
				curr = next;
				if (p->backface) {
					next = (next - 1 + p->num_vertices) %
					       p->num_vertices;
				} else {
					next = (next + 1) % p->num_vertices;
				}
			}
			if (next != curr) {
				setup_polygon_slope(&pinfo->left_slope,
						p->vertices[curr],
						p->vertices[next],
						p->normalized_w[curr],
						p->normalized_w[next]);
				pinfo->prev_left = curr;
				pinfo->left = next;
			}
		}
		if (p->vertices[pinfo->right]->sy == scanline) {
			curr = pinfo->right;
			next = pinfo->right;
			while (next != pinfo->end &&
					p->vertices[next]->sy <= scanline) {
				curr = next;
				if (p->backface) {
					next = (next + 1) % p->num_vertices;
				} else {
					next = (next - 1 + p->num_vertices) %
					       p->num_vertices;
				}
			}
			if (next != curr) {
				setup_polygon_slope(&pinfo->right_slope,
						p->vertices[curr],
						p->vertices[next],
						p->normalized_w[curr],
						p->normalized_w[next]);
				pinfo->prev_right = curr;
				pinfo->right = next;
			}
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
			flip = flip && s & ~(size - 1) & size;
			s &= size - 1;
			if (flip) {
				s = (size - s) & (size - 1);
			}
		}
	}

	return s;
}

static void
draw_pixel(gpu_3d_engine *gpu, polygon *p, s32 y, u32 x, s32 a, s32 r, s32 g,
		s32 b, s32 s, s32 t)
{
	u32 final_color;
	u32 mode = p->attr >> 4 & 3;
	u32 tx_format = p->tx_param >> 26 & 7;
	bool color_0_transparent = p->tx_param & BIT(29);
	u8 tx_color[4]{};

	if (tx_format == 0) {
		final_color = a << 18 | b << 12 | g << 6 | r;
		gpu->color_buf[y][x] = final_color;
		return;
	}

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
	case 7:
	{
		u32 offset = base_offset;
		offset += (t >> 4) * s_size << 1;
		offset += (s >> 4) << 1;
		u16 color = vram_read_texture<u16>(gpu->nds, offset);
		unpack_abgr1555_3d(color, tx_color);
		break;
	}
	}

	final_color = (u32)tx_color[3] << 18 | (u32)tx_color[2] << 12 |
	              (u32)tx_color[1] << 6 | tx_color[0];
	gpu->color_buf[y][x] = final_color;
}

static s32
get_depth(polygon *p, interpolator *i, s32 zl, s32 zr)
{
	s32 z = interpolate(i, zl, zr);
	if (p->wbuffering) {
		if (p->wshift >= 0) {
			return z << p->wshift;
		} else {
			return z >> -p->wshift;
		}
	} else {
		return z;
	}
}

static void
render_polygon_scanline(gpu_3d_engine *gpu, s32 scanline, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pinfo = &gpu->re.poly_info[poly_num];

	if (polygon_not_in_scanline(gpu, scanline, poly_num))
		return;

	slope *sl = &pinfo->left_slope;
	slope *sr = &pinfo->right_slope;

	s32 xstart[2];
	s32 xend[2];
	get_slope_x_start_end(sl, sr, xstart, xend, scanline);

	if (xstart[0] >= xend[1]) {
		std::swap(sl, sr);
		std::swap(xstart[0], xend[0]);
		std::swap(xstart[1], xend[1]);
	}

	set_slope_interp_x(sl, scanline, xstart[0]);
	set_slope_interp_x(sr, scanline, xend[1]);
	interp_update_w(&sl->interp);
	interp_update_w(&sr->interp);
	s32 zl = interpolate(&sl->interp, p->z[pinfo->prev_left],
			p->z[pinfo->left]);
	s32 rl = interpolate(&sl->interp, sl->v0->color.r, sl->v1->color.r);
	s32 gl = interpolate(&sl->interp, sl->v0->color.g, sl->v1->color.g);
	s32 bl = interpolate(&sl->interp, sl->v0->color.b, sl->v1->color.b);
	s32 tx_sl = interpolate(&sl->interp, sl->v0->tx.s, sl->v1->tx.s);
	s32 tx_tl = interpolate(&sl->interp, sl->v0->tx.t, sl->v1->tx.t);
	s32 zr = interpolate(&sr->interp, p->z[pinfo->prev_right],
			p->z[pinfo->right]);
	s32 rr = interpolate(&sr->interp, sr->v0->color.r, sr->v1->color.r);
	s32 gr = interpolate(&sr->interp, sr->v0->color.g, sr->v1->color.g);
	s32 br = interpolate(&sr->interp, sr->v0->color.b, sr->v1->color.b);
	s32 tx_sr = interpolate(&sr->interp, sr->v0->tx.s, sr->v1->tx.s);
	s32 tx_tr = interpolate(&sr->interp, sr->v0->tx.t, sr->v1->tx.t);

	interpolator span;
	interp_setup(&span, xstart[0], xend[1] - 1, sl->interp.w,
			sr->interp.w);

	s32 alpha = p->attr >> 16 & 0x1F;
	if (alpha == 0) {
		alpha = 31;
	}

	s32 draw_x = std::max(0, xstart[0]);
	s32 draw_x_end = std::min(256, xstart[1]);
	for (s32 x = draw_x; x < draw_x_end; x++) {
		span.x = x;
		interp_update_w(&span);
		s32 depth = get_depth(p, &span, zl, zr);
		if (depth > gpu->depth_buf[scanline][x])
			continue;
		s32 r = interpolate(&span, rl, rr);
		s32 g = interpolate(&span, gl, gr);
		s32 b = interpolate(&span, bl, br);
		s32 tx_s = interpolate(&span, tx_sl, tx_sr);
		s32 tx_t = interpolate(&span, tx_tl, tx_tr);

		draw_pixel(gpu, p, scanline, x, alpha, r, g, b, tx_s, tx_t);
		gpu->depth_buf[scanline][x] = depth;
	}

	for (s32 x = xstart[1]; x < xend[0]; x++) {
		if (x < 0 || x >= 256)
			continue;
		span.x = x;
		interp_update_w(&span);
		s32 depth = get_depth(p, &span, zl, zr);
		if (depth > gpu->depth_buf[scanline][x])
			continue;
		s32 r = interpolate(&span, rl, rr);
		s32 g = interpolate(&span, gl, gr);
		s32 b = interpolate(&span, bl, br);
		s32 tx_s = interpolate(&span, tx_sl, tx_sr);
		s32 tx_t = interpolate(&span, tx_tl, tx_tr);

		draw_pixel(gpu, p, scanline, x, alpha, r, g, b, tx_s, tx_t);
		gpu->depth_buf[scanline][x] = depth;
	}

	draw_x = std::max(0, xend[0]);
	draw_x_end = std::min(256, xend[1]);
	for (s32 x = draw_x; x < draw_x_end; x++) {
		span.x = x;
		interp_update_w(&span);
		s32 depth = get_depth(p, &span, zl, zr);
		if (depth > gpu->depth_buf[scanline][x])
			continue;
		s32 r = interpolate(&span, rl, rr);
		s32 g = interpolate(&span, gl, gr);
		s32 b = interpolate(&span, bl, br);
		s32 tx_s = interpolate(&span, tx_sl, tx_sr);
		s32 tx_t = interpolate(&span, tx_tl, tx_tr);

		draw_pixel(gpu, p, scanline, x, alpha, r, g, b, tx_s, tx_t);
		gpu->depth_buf[scanline][x] = depth;
	}
}

static void
render_3d_scanline(gpu_3d_engine *gpu, u32 scanline)
{
	auto& re = gpu->re;

	for (u32 i = 0; i < re.num_polygons; i++) {
		setup_polygon_scanline(gpu, scanline, i);
		render_polygon_scanline(gpu, scanline, i);
	}
}

static u32
axbgr_51555_to_abgr5666(u32 s)
{
	u32 r = s & 0x1F;
	u32 g = s >> 5 & 0x1F;
	u32 b = s >> 10 & 0x1F;
	u32 a = s >> 16 & 0x1F;

	if (r)
		r = (r << 1) + 1;
	if (g)
		g = (g << 1) + 1;
	if (b)
		b = (b << 1) + 1;

	return a << 18 | b << 12 | g << 6 | r;
}

static void
clear_buffers(gpu_3d_engine *gpu)
{
	auto& re = gpu->re;

	if (!(re.r.disp3dcnt & BIT(14))) {
		u32 color = axbgr_51555_to_abgr5666(re.r.clear_color);
		u32 attr = re.r.clear_color & 0x3F008000;
		u32 depth = 0x200 * re.r.clear_depth + 0x1FF;

		for (u32 i = 0; i < 192; i++) {
			for (u32 j = 0; j < 256; j++) {
				gpu->color_buf[i][j] = color;
				gpu->depth_buf[i][j] = depth;
				gpu->attr_buf[i][j] = attr;
			}
		}
	} else {
		/* TODO: */
	}
}

static void
find_polygon_start_end(polygon *p, u32 *start_r, u32 *end_r)
{
	if (p->num_vertices <= 1) {
		*start_r = 0;
		*end_r = 0;
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

	*start_r = start;
	*end_r = end;
}

static void
setup_polygons(gpu_3d_engine *gpu)
{
	auto& re = gpu->re;
	auto& pr = gpu->poly_ram[gpu->re_buf];

	re.num_polygons = pr.count;

	/* TODO: sort polygons */
	for (u32 i = 0; i < pr.count; i++) {
		re.polygons[i] = &pr.polygons[i];
	}

	for (u32 i = 0; i < pr.count; i++) {
		find_polygon_start_end(re.polygons[i], &re.poly_info[i].start,
				&re.poly_info[i].end);
		setup_polygon_initial_slope(gpu, i);
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

} // namespace twice
