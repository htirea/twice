#include "nds/mem/vram.h"
#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

using vertex = gpu_3d_engine::vertex;
using polygon = gpu_3d_engine::polygon;
using polygon_info = gpu_3d_engine::rendering_engine::polygon_info;
using slope = gpu_3d_engine::rendering_engine::slope;
using interpolator = gpu_3d_engine::rendering_engine::interpolator;

static void clear_buffers(gpu_3d_engine *);
static void setup_polygons(gpu_3d_engine *);
static void render_scanline(gpu_3d_engine *, s32 y);
static void apply_effects_scanline(gpu_3d_engine *, s32 y);

void
gpu3d_render_frame(gpu_3d_engine *gpu)
{
	setup_fast_texture_vram(gpu->nds);
	clear_buffers(gpu);
	setup_polygons(gpu);

	for (s32 i = 0; i < 192; i++) {
		render_scanline(gpu, i);
	}

	for (s32 i = 0; i < 192; i++) {
		apply_effects_scanline(gpu, i);
	}
}

static void clear_stencil_buffer(gpu_3d_engine *gpu);

static void
clear_buffers(gpu_3d_engine *gpu)
{
	auto& re = gpu->re;

	u32 attr = re.r.clear_color & 0x3F008000;
	u32 fog = attr >> 15 & 3;
	u32 opaque_id = attr >> 24 & 0x3F;
	u32 depth = 0x200 * re.r.clear_depth + 0x1FF;

	if (!(re.r.disp3dcnt & BIT(14))) {
		u8 rgba[4];
		unpack_bgr555_3d(re.r.clear_color, rgba);
		rgba[3] = re.r.clear_color >> 16 & 0x1F;
		color4 color = { rgba[0], rgba[1], rgba[2], rgba[3] };

		for (u32 i = 0; i < 192; i++) {
			for (u32 j = 0; j < 256; j++) {
				gpu->color_buf[0][i][j] = color;
				gpu->depth_buf[0][i][j] = depth;
				gpu->attr_buf[0][i][j].fog = fog;
				gpu->attr_buf[0][i][j].translucent_id = 0;
				gpu->attr_buf[0][i][j].translucent = 0;
				gpu->attr_buf[0][i][j].backface = 0;
				gpu->attr_buf[0][i][j].opaque_id = opaque_id;

				gpu->color_buf[1][i][j] = color;
				gpu->depth_buf[1][i][j] = depth;
				gpu->attr_buf[1][i][j].fog = fog;
				gpu->attr_buf[1][i][j].translucent_id = 0;
				gpu->attr_buf[1][i][j].translucent = 0;
				gpu->attr_buf[1][i][j].backface = 0;
				gpu->attr_buf[1][i][j].opaque_id = opaque_id;
			}
		}

		clear_stencil_buffer(gpu);
	} else {
		/* TODO: */
		LOG("clear buffer bitmap\n");
	}

	re.outside_opaque_id = opaque_id;
	re.outside_depth = depth;
}

static void
clear_stencil_buffer(gpu_3d_engine *gpu)
{
	for (u32 i = 0; i < 256; i++) {
		gpu->stencil_buf[0][i] = 0;
		gpu->stencil_buf[1][i] = 0;
	}
}

static void find_polygon_start_end_sortkey(polygon *p, bool manual_sort);
static void setup_polygon(gpu_3d_engine *, u32 poly_num);

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
	std::stable_sort(re.polygons, re.polygons + pr.count,
			[](const polygon *a, const polygon *b) {
				return a->sortkey < b->sortkey;
			});

	for (u32 i = 0; i < pr.count; i++) {
		setup_polygon(gpu, i);
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

static void set_polygon_bounds(polygon *, polygon_info *);

static void
setup_polygon(gpu_3d_engine *gpu, u32 poly_num)
{
	auto& re = gpu->re;
	polygon *p = re.polygons[poly_num];
	polygon_info *pi = &re.poly_info[poly_num];

	set_polygon_bounds(p, pi);
	pi->poly_id = p->attr >> 24 & 0x3F;
	pi->shadow = (p->attr >> 4 & 3) == 3;
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

	pi->left = p->start;
	pi->prev_left = p->start;
	pi->right = p->start;
	pi->prev_right = p->start;

	s32 top_count = 0;
	s32 bottom_count = 0;
	for (u32 i = 0; i < p->num_vertices; i++) {
		vertex *v = p->vertices[i];
		if (v->sy == y_start) {
			top_count++;
		}
		if (v->sy == y_end) {
			bottom_count++;
		}
	}

	pi->top_edge = top_count > 1 ? y_start : -1;
	pi->bottom_edge = bottom_count > 1 ? y_end - 1 : -1;
}

static void setup_polygon_scanline(gpu_3d_engine *, s32 y, u32 poly_num);
static void render_polygon_scanline(gpu_3d_engine *, s32 y, u32 poly_num);

static void
render_scanline(gpu_3d_engine *gpu, s32 y)
{
	auto& re = gpu->re;

	for (u32 i = 0; i < re.num_polygons; i++) {
		polygon_info *pi = &re.poly_info[i];
		if (y < pi->y_start || y >= pi->y_end)
			continue;
		setup_polygon_scanline(gpu, y, i);
		render_polygon_scanline(gpu, y, i);
	}
}

static void setup_polygon_slope(
		slope *s, vertex *v0, vertex *v1, s32 w0, s32 w1, bool left);

static void
setup_polygon_scanline(gpu_3d_engine *gpu, s32 y, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pi = &gpu->re.poly_info[poly_num];
	slope *sl = &pi->left_slope;
	slope *sr = &pi->right_slope;

	u32 curr, next;

	if (p->vertices[pi->left]->sy == y) {
		curr = pi->left;
		next = pi->left;
		while (next != p->end && p->vertices[next]->sy <= y) {
			curr = next;
			if (p->backface) {
				next = (next - 1 + p->num_vertices) %
				       p->num_vertices;
			} else {
				next = (next + 1) % p->num_vertices;
			}
		}
		if (next != curr) {
			setup_polygon_slope(sl, p->vertices[curr],
					p->vertices[next],
					p->normalized_w[curr],
					p->normalized_w[next], true);
			pi->prev_left = curr;
			pi->left = next;
		}
	}
	if (p->vertices[pi->right]->sy == y) {
		curr = pi->right;
		next = pi->right;
		while (next != p->end && p->vertices[next]->sy <= y) {
			curr = next;
			if (p->backface) {
				next = (next + 1) % p->num_vertices;
			} else {
				next = (next - 1 + p->num_vertices) %
				       p->num_vertices;
			}
		}
		if (next != curr) {
			setup_polygon_slope(sr, p->vertices[curr],
					p->vertices[next],
					p->normalized_w[curr],
					p->normalized_w[next], false);
			pi->prev_right = curr;
			pi->right = next;
		}
	}

	sl->line = sr->line =
			sl->v0->sx == sr->v0->sx && sl->v0->sy == sr->v0->sy &&
			sl->v1->sx == sr->v1->sx && sl->v1->sy == sr->v1->sy;
}

static void interp_setup(
		interpolator *i, s32 x0, s32 x1, s32 w0, s32 w1, bool edge);

/* slope algorithm taken from: https://github.com/StrikerX3/nds-interp */
static void
setup_polygon_slope(
		slope *s, vertex *v0, vertex *v1, s32 w0, s32 w1, bool left)
{
	s32 one = (s32)1 << 18;
	s32 half = (s32)1 << 17;

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

	s->dx = x1 - x0;
	s->dy = y1 - y0;
	s->xmajor = s->dx > s->dy;
	s->wide = s->dx >= s->dy << 1;

	if (s->dx >= s->dy) {
		s->x0 = s->negative ? s->x0 - half : s->x0 + half;
	}

	if (s->dx == 0 || s->dy == 0) {
		s->m = 0;
	} else {
		s->m = s->dx * (one / s->dy);
	}

	s->vertical = s->m == 0;
	s->left = left;

	bool adjust = (s->dx >= s->dy) && (s->left == s->negative);
	interp_setup(&s->interp, v0->sy - adjust, v1->sy - adjust, w0, w1,
			true);
}

static void
interp_setup(interpolator *i, s32 x0, s32 x1, s32 w0, s32 w1, bool edge)
{
	i->x0 = x0;
	i->x1 = x1;
	i->w0 = w0;
	i->w1 = w1;

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

struct render_polygon_ctx {
	polygon *p;
	polygon_info *pi;
	s32 attr_l[5];
	s32 attr_r[5];
	s32 zl;
	s32 zr;
	s32 cov_l[2];
	s32 cov_r[2];
	u8 alpha;
	u8 alpha_test_ref;
	bool alpha_blending;
	bool antialiasing;

	s32 cov[2];
	s32 cov_x[2];
};

static void get_slope_x_start_end(slope *sl, slope *sr, s32 *xl, s32 *xr,
		s32 *cov_l, s32 *cov_r, s32 y);
static void interp_update_x(interpolator *i, s32 x);
static s32 interpolate(interpolator *i, s32 y0, s32 y1);
static void interpolate_multiple(interpolator *i, s32 *y0, s32 *y1, s32 *y);
static s32 interpolate_z(interpolator *i, s32 z0, s32 z1, bool wbuffering);
static void render_shadow_mask_polygon_pixel(gpu_3d_engine *gpu,
		render_polygon_ctx *ctx, interpolator *span, s32 x, s32 y);
static void render_polygon_pixel(gpu_3d_engine *gpu, render_polygon_ctx *,
		interpolator *span, s32 x, s32 y, bool edge);

static void
render_polygon_scanline(gpu_3d_engine *gpu, s32 y, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pi = &gpu->re.poly_info[poly_num];
	render_polygon_ctx ctx;
	ctx.p = p;
	ctx.pi = pi;

	bool shadow_mask = pi->shadow && pi->poly_id == 0;
	if (shadow_mask && !gpu->re.last_poly_is_shadow_mask) {
		clear_stencil_buffer(gpu);
	}
	gpu->re.last_poly_is_shadow_mask = shadow_mask;

	slope *sl = &pi->left_slope;
	slope *sr = &pi->right_slope;
	s32 zl[2] = { p->z[pi->prev_left], p->z[pi->left] };
	s32 zr[2] = { p->z[pi->prev_right], p->z[pi->right] };

	s32 xl[2], xr[2], xm[2], cov_l[2], cov_r[2];
	get_slope_x_start_end(sl, sr, xl, xr, cov_l, cov_r, y);

	bool swapped = false;
	if (xl[0] >= xr[1]) {
		swapped = true;
		std::swap(sl, sr);
		std::swap(xl[0], xr[0]);
		std::swap(xl[1], xr[1]);
		std::swap(cov_l[0], cov_r[0]);
		std::swap(cov_l[1], cov_r[1]);
		std::swap(zl[0], zr[0]);
		std::swap(zl[1], zr[1]);

		/* TODO: break antialiasing */
		cov_l[0] ^= 0x3FF;
		cov_l[1] ^= 0x3FF;
		cov_r[0] ^= 0x3FF;
		cov_r[1] ^= 0x3FF;
	}

	bool fill_vertical_right_edge = false;
	if (sr->vertical && !swapped) {
		xr[0]--;
		xr[1]--;
		fill_vertical_right_edge = true;
	}

	bool fill_vertical_left_edge = false;
	if (sl->vertical && swapped) {
		xl[0]--;
		xl[1]--;
		fill_vertical_left_edge = true;
	}

	xr[0] = std::max(xr[0], xl[1]);
	xm[0] = xl[1];
	xm[1] = xr[0];

	if (swapped) {
		if (sl->xmajor) {
			xl[0] = xl[1] - 1;
		}
		if (sr->xmajor) {
			xr[1] = xr[0] + 1;
		}
	}

	interp_update_x(&sl->interp, y);
	interp_update_x(&sr->interp, y);
	s32 wl = interpolate(&sl->interp, sl->interp.w0, sl->interp.w1);
	s32 wr = interpolate(&sr->interp, sr->interp.w0, sr->interp.w1);
	ctx.zl = interpolate_z(&sl->interp, zl[0], zl[1], p->wbuffering);
	ctx.zr = interpolate_z(&sr->interp, zr[0], zr[1], p->wbuffering);

	if (!shadow_mask) {
		interpolate_multiple(&sl->interp, sl->v0->attr, sl->v1->attr,
				ctx.attr_l);
		interpolate_multiple(&sr->interp, sr->v0->attr, sr->v1->attr,
				ctx.attr_r);
	}

	interpolator span;
	interp_setup(&span, xl[0], xr[1], wl, wr, false);

	u32 alpha = p->attr >> 16 & 0x1F;
	bool wireframe = alpha == 0;
	bool translucent = alpha != 0 && alpha != 31;
	bool alpha_test = gpu->re.r.disp3dcnt & BIT(2);
	bool alpha_blending = gpu->re.r.disp3dcnt & BIT(3);
	bool antialiasing = gpu->re.r.disp3dcnt & BIT(4);
	bool edge_marking = gpu->re.r.disp3dcnt & BIT(5);
	bool bottom_edge_fill_rule =
			y == pi->bottom_edge && sl->v1->sx != sr->v1->sx;
	bool force_fill_edge = wireframe || (translucent && alpha_blending) ||
	                       antialiasing || edge_marking;

	ctx.alpha = alpha == 0 ? 31 : alpha;
	ctx.alpha_test_ref = alpha_test ? gpu->re.r.alpha_test_ref : 0;
	ctx.alpha_blending = alpha_blending;
	ctx.antialiasing = antialiasing;

	bool fill_left = sl->negative || !sl->xmajor || force_fill_edge ||
	                 fill_vertical_left_edge || sl->line ||
	                 (sl->xmajor && bottom_edge_fill_rule);
	if (fill_left) {
		s32 start = std::max(0, xl[0]);
		s32 end = std::min(256, xl[1]);

		for (s32 x = start; shadow_mask && x < end; x++) {
			render_shadow_mask_polygon_pixel(
					gpu, &ctx, &span, x, y);
		}

		if (!shadow_mask) {
			ctx.cov[0] = cov_l[0];
			ctx.cov[1] = cov_l[1];
			ctx.cov_x[0] = xl[0];
			ctx.cov_x[1] = xl[1] - 1;
		}

		for (s32 x = start; !shadow_mask && x < end; x++) {
			render_polygon_pixel(gpu, &ctx, &span, x, y, true);
		}
	}

	bool edge = y == pi->top_edge || y == pi->bottom_edge;
	bool fill_middle = !wireframe || edge;
	if (fill_middle) {
		s32 start = std::max(0, xm[0]);
		s32 end = std::min(256, xm[1]);

		for (s32 x = start; shadow_mask && x < end; x++) {
			render_shadow_mask_polygon_pixel(
					gpu, &ctx, &span, x, y);
		}

		if (!shadow_mask) {
			ctx.cov[0] = 0x3FF;
			ctx.cov[1] = 0x3FF;
			ctx.cov_x[0] = 0;
			ctx.cov_x[1] = 0;
		}

		for (s32 x = start; !shadow_mask && x < end; x++) {
			render_polygon_pixel(gpu, &ctx, &span, x, y, edge);
		}
	}

	bool fill_right = (!sr->negative && sr->xmajor) ||
	                  fill_vertical_right_edge || force_fill_edge ||
	                  (sr->xmajor && bottom_edge_fill_rule);
	if (fill_right) {
		s32 start = std::max(0, xr[0]);
		s32 end = std::min(256, xr[1]);

		for (s32 x = start; shadow_mask && x < end; x++) {
			render_shadow_mask_polygon_pixel(
					gpu, &ctx, &span, x, y);
		}

		if (antialiasing && !shadow_mask) {
			ctx.cov[0] = cov_r[0];
			ctx.cov[1] = cov_r[1];
			ctx.cov_x[0] = xr[0];
			ctx.cov_x[1] = xr[1] - 1;
		}

		for (s32 x = start; !shadow_mask && x < end; x++) {
			render_polygon_pixel(gpu, &ctx, &span, x, y, true);
		}
	}
}

static void get_slope_x(slope *s, s32 *x, s32 *cov, s32 y);

static void
get_slope_x_start_end(slope *sl, slope *sr, s32 *xl, s32 *xr, s32 *cov_l,
		s32 *cov_r, s32 y)
{
	get_slope_x(sl, xl, cov_l, y);
	get_slope_x(sr, xr, cov_r, y);
}

static void
get_slope_x(slope *s, s32 *x, s32 *cov, s32 y)
{
	s32 one = (s32)1 << 18;
	s32 dy = y - s->y0;

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

	if (s->vertical || s->dy == 0) {
		cov[0] = 0x3FF;
		cov[1] = 0x3FF;
	} else if (s->dx == s->dy) {
		cov[0] = 0x1FF;
		cov[1] = 0x1FF;
	} else if (s->xmajor) {
		s32 x0 = x[0] >> 18;
		s32 x1 = x[1] >> 18;

		cov[0] = (x0 - s->v0->sx) * (s->dy << 10) / s->dx & 0x3FF;
		cov[1] = (x1 - s->v0->sx) * (s->dy << 10) / s->dx & 0x3FF;

		if (s->wide) {
			if (cov[0] >= 0x200 && x0 != x1) {
				cov[0] ^= 0x3FF;
			}
		} else {
			cov[0] = (cov[0] + 0x200) & 0x3FF;
			if (x0 != x1) {
				cov[1] = 0x3FF;
			}
		}

		if (!s->left) {
			cov[0] ^= 0x3FF;
			cov[1] ^= 0x3FF;
		}
	} else {
		cov[0] = x[0] >> 8 & 0x3FF;
		cov[1] = cov[0];

		if (s->left) {
			cov[0] ^= 0x3FF;
			cov[1] ^= 0x3FF;
		}
	}

	x[0] >>= 18;
	x[1] >>= 18;
	x[1]++;
}

static void
interp_update_perspective_factor(interpolator *i)
{
	s64 numer = ((s64)i->w0_n << i->precision) * ((s64)i->x - i->x0);
	s64 denom = (i->w1_d * ((s64)i->x1 - i->x) +
			i->w0_d * ((s64)i->x - i->x0));

	i->pfactor = denom == 0 ? 0 : numer / denom;
	i->pfactor_r = ((u32)1 << i->precision) - i->pfactor;
}

static void
interp_update_x(interpolator *i, s32 x)
{
	i->x = x;
	interp_update_perspective_factor(i);
}

static s32
interpolate_linear(interpolator *i, s32 y0, s32 y1)
{
	return ilerp(y0, y1, i->x0, i->x1, i->x);
}

static void
interpolate_linear_multiple(interpolator *i, s32 *y0, s32 *y1, s32 *y)
{
	for (u32 j = 0; j < 5; j++) {
		y[j] = interpolate_linear(i, y0[j], y1[j]);
	}
}

static s32
interpolate_perspective(interpolator *i, s32 y0, s32 y1)
{
	if (y0 <= y1) {
		return y0 + ((y1 - y0) * i->pfactor >> i->precision);
	} else {
		return y1 + ((y0 - y1) * i->pfactor_r >> i->precision);
	}
}

static void
interpolate_perspective_multiple(interpolator *i, s32 *y0, s32 *y1, s32 *y)
{
	for (u32 j = 0; j < 5; j++) {
		y[j] = interpolate_perspective(i, y0[j], y1[j]);
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

static bool depth_test(gpu_3d_engine *gpu, render_polygon_ctx *ctx, s32 z,
		s32 x, s32 y, bool layer);

static void
render_shadow_mask_polygon_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		interpolator *span, s32 x, s32 y)
{
	interp_update_x(span, x);
	s32 z = interpolate_z(span, ctx->zl, ctx->zr, ctx->p->wbuffering);

	if (ctx->alpha <= ctx->alpha_test_ref) {
		return;
	}

	if (!depth_test(gpu, ctx, z, x, y, 0)) {
		gpu->stencil_buf[0][x] = true;
	}

	if (gpu->attr_buf[0][y][x].edge) {
		if (!depth_test(gpu, ctx, z, x, y, 1)) {
			gpu->stencil_buf[1][x] = true;
		}
	}
}

static color4 get_pixel_color(gpu_3d_engine *gpu, polygon *p, u8 rv, u8 gv,
		u8 bv, u8 av, s32 s, s32 t);
static void draw_opaque_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		color4 color, s32 x, s32 y, s32 z, bool edge, bool layer);
static void draw_translucent_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		color4 color, s32 x, s32 y, s32 z, bool edge, bool layer);

static void
render_polygon_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		interpolator *span, s32 x, s32 y, bool edge)
{
	bool layer = 0;
	bool shadow = ctx->pi->shadow;

	if (shadow) {
		if (!gpu->stencil_buf[0][x] && !gpu->stencil_buf[1][x])
			return;
		if (!gpu->stencil_buf[0][x]) {
			layer = 1;
		}
	}

	interp_update_x(span, x);

	s32 z = interpolate_z(span, ctx->zl, ctx->zr, ctx->p->wbuffering);
	if (!depth_test(gpu, ctx, z, x, y, layer)) {
		if (layer == 1 || !gpu->attr_buf[0][y][x].edge)
			return;

		layer = 1;
		if (!depth_test(gpu, ctx, z, x, y, 1))
			return;
	}

	if (shadow) {
		auto& attr = gpu->attr_buf[layer][y][x];
		u32 dest_id = attr.translucent ? attr.translucent_id
		                               : attr.opaque_id;
		if (ctx->pi->poly_id == dest_id)
			return;
	}

	s32 attr_result[5];
	interpolate_multiple(span, ctx->attr_l, ctx->attr_r, attr_result);

	color4 px_color = get_pixel_color(gpu, ctx->p, attr_result[0],
			attr_result[1], attr_result[2], ctx->alpha,
			attr_result[3], attr_result[4]);
	if (px_color.a <= ctx->alpha_test_ref)
		return;

	if (layer == 0) {
		gpu->color_buf[1][y][x] = gpu->color_buf[0][y][x];
		gpu->depth_buf[1][y][x] = gpu->depth_buf[0][y][x];
		gpu->attr_buf[1][y][x] = gpu->attr_buf[0][y][x];
	}

	if (px_color.a == 31) {
		draw_opaque_pixel(gpu, ctx, px_color, x, y, z, edge, layer);
		if (edge && ctx->antialiasing && layer == 0) {
			s32 cov = ilerp(ctx->cov[0], ctx->cov[1],
					ctx->cov_x[0], ctx->cov_x[1], x);
			gpu->attr_buf[0][y][x].coverage = cov >> 5;
		}
	} else {
		draw_translucent_pixel(
				gpu, ctx, px_color, x, y, z, edge, layer);
	}
}

static bool
depth_test(gpu_3d_engine *gpu, render_polygon_ctx *ctx, s32 z, s32 x, s32 y,
		bool layer)
{
	s32 z_dest = gpu->depth_buf[layer][y][x];
	auto attr_dest = gpu->attr_buf[layer][y][x];

	if (ctx->p->attr & BIT(14)) {
		s32 margin = ctx->p->wbuffering ? 0xFF : 0x200;
		return z <= z_dest + margin;
	}

	/* TODO: wireframe edge */

	if (!ctx->p->backface &&
			(!attr_dest.translucent && attr_dest.backface)) {
		return z <= z_dest;
	}

	return z < z_dest;
}

static color4 get_texture_color(gpu_3d_engine *gpu, polygon *p, s32 s, s32 t);

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

static s32 clamp_or_repeat_texcoords(s32 s, s32 size, bool clamp, bool flip);
static void blend_bgr555_11_3d(u16 color0, u16 color1, u8 *color_out);
static void blend_bgr555_53_3d(u16 color0, u16 color1, u8 *color_out);

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
draw_opaque_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx, color4 color,
		s32 x, s32 y, s32 z, bool edge, bool layer)
{
	gpu->color_buf[layer][y][x] = color;
	gpu->depth_buf[layer][y][x] = z;

	auto& attr = gpu->attr_buf[layer][y][x];
	attr.edge = edge;
	attr.fog = ctx->p->attr >> 15 & 1;
	attr.translucent = 0;
	attr.backface = ctx->p->backface;
	attr.opaque_id = ctx->p->attr >> 24 & 0x3F;
}

static color4 alpha_blend(color4 c0, color4 c1);

static void
draw_translucent_pixel(gpu_3d_engine *gpu, render_polygon_ctx *ctx,
		color4 color, s32 x, s32 y, s32 z, bool edge, bool layer)
{
	u32 poly_id = ctx->p->attr >> 24 & 0x3F;
	auto& attr = gpu->attr_buf[layer][y][x];
	if (attr.translucent && attr.translucent_id == poly_id)
		return;

	auto& dst_color = gpu->color_buf[layer][y][x];
	if (dst_color.a == 0) {
		dst_color = color;
	} else if (!ctx->alpha_blending) {
		dst_color.r = color.r;
		dst_color.g = color.g;
		dst_color.b = color.b;
		dst_color.a = std::max(dst_color.a, color.a);
	} else {
		dst_color = alpha_blend(color, dst_color);
	}

	if (ctx->p->attr & BIT(11)) {
		gpu->depth_buf[layer][y][x] = z;
	}

	attr.edge = edge;
	attr.fog &= ctx->p->attr >> 15 & 1;
	attr.translucent_id = poly_id;
	attr.translucent = 1;
	attr.backface = ctx->p->backface;
}

static color4
alpha_blend(color4 c0, color4 c1)
{
	u8 r = ((c0.a + 1) * c0.r + (31 - c0.a) * c1.r) >> 5;
	u8 g = ((c0.a + 1) * c0.g + (31 - c0.a) * c1.g) >> 5;
	u8 b = ((c0.a + 1) * c0.b + (31 - c0.a) * c1.b) >> 5;
	u8 a = std::max(c0.a, c1.a);

	return { r, g, b, a };
}

static void apply_edge_marking(gpu_3d_engine *gpu, s32 y);
static void apply_fog(gpu_3d_engine *gpu, s32 y);
static void apply_antialiasing(gpu_3d_engine *gpu, s32 y);

static void
apply_effects_scanline(gpu_3d_engine *gpu, s32 y)
{

	if (gpu->re.r.disp3dcnt & BIT(5)) {
		apply_edge_marking(gpu, y);
	}

	if (gpu->re.r.disp3dcnt & BIT(7)) {
		apply_fog(gpu, y);
	}

	if (gpu->re.r.disp3dcnt & BIT(4)) {
		apply_antialiasing(gpu, y);
	}
}

static void get_surrounding_id_depth(
		gpu_3d_engine *gpu, s32 y, s32 x, u32 *id_out, s32 *depth_out);

static void
apply_edge_marking(gpu_3d_engine *gpu, s32 y)
{
	auto& re = gpu->re;

	for (u32 x = 0; x < 256; x++) {
		auto attr = gpu->attr_buf[0][y][x];

		if (attr.translucent || !attr.edge)
			continue;

		u32 adj_ids[4];
		s32 adj_depths[4];
		get_surrounding_id_depth(gpu, y, x, adj_ids, adj_depths);

		u32 id = attr.opaque_id;
		s32 depth = gpu->depth_buf[0][y][x];

		bool mark = false;
		for (u32 i = 0; i < 4; i++) {
			if (adj_ids[i] != id && depth < adj_depths[i]) {
				mark = true;
			}
		}

		if (!mark)
			continue;

		u16 color = readarr<u16>(re.r.edge_color, id >> 2 & ~1);
		u8 r, g, b;
		unpack_bgr555_3d(color, &r, &g, &b);
		gpu->color_buf[0][y][x].r = r;
		gpu->color_buf[0][y][x].g = g;
		gpu->color_buf[0][y][x].b = b;
		gpu->attr_buf[0][y][x].coverage = 0x10;
	}
}

static void
get_surrounding_id_depth(
		gpu_3d_engine *gpu, s32 y, s32 x, u32 *id_out, s32 *depth_out)
{
	auto& re = gpu->re;

	s32 offsets[4][2] = { { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 } };
	for (u32 i = 0; i < 4; i++) {
		s32 x2 = x + offsets[i][0];
		s32 y2 = y + offsets[i][1];

		if (x2 < 0 || x2 >= 256 || y2 < 0 || y2 >= 192) {
			id_out[i] = re.outside_opaque_id;
			depth_out[i] = re.outside_depth;
		} else {
			id_out[i] = gpu->attr_buf[0][y2][x2].opaque_id;
			depth_out[i] = gpu->depth_buf[0][y2][x2];
		}
	}
}

static u8 calculate_fog_density(
		u8 *fog_table, s32 fog_offset, s32 fog_step, s32 z);

static void
apply_fog(gpu_3d_engine *gpu, s32 y)
{
	auto& re = gpu->re;

	u16 fog_shift = re.r.disp3dcnt >> 8 & 0xF;
	s32 fog_step = 0x400 >> fog_shift;
	u16 fog_offset = re.r.fog_offset;
	bool alpha_only = re.r.disp3dcnt & BIT(6);

	u8 rf, gf, bf;
	unpack_bgr555_3d(re.r.fog_color, &rf, &gf, &bf);
	u8 af = re.r.fog_color >> 16 & 0x1F;

	for (s32 x = 0; x < 256; x++) {
		for (int layer = 0; layer < 2; layer++) {
			if (!gpu->attr_buf[layer][y][x].fog)
				continue;

			u32 z = gpu->depth_buf[layer][y][x] >> 9;
			u8 t = calculate_fog_density(re.r.fog_table,
					fog_offset, fog_step, z);
			if (t >= 127) {
				t = 128;
			}

			auto& dst = gpu->color_buf[layer][y][x];
			dst.a = (af * t + dst.a * (128 - t)) >> 7;
			if (!alpha_only) {
				dst.r = (rf * t + dst.r * (128 - t)) >> 7;
				dst.g = (gf * t + dst.g * (128 - t)) >> 7;
				dst.b = (bf * t + dst.b * (128 - t)) >> 7;
			}
		}
	}
}

static u8
calculate_fog_density(u8 *fog_table, s32 fog_offset, s32 fog_step, s32 z)
{
	if (z < fog_offset + fog_step)
		return fog_table[0];

	if (z >= fog_offset + (fog_step * 32))
		return fog_table[31];

	s32 idx_l = (z - fog_offset) / fog_step - 1;
	s32 idx_r = (z - fog_offset + fog_step - 1) / fog_step - 1;

	if (idx_l == idx_r)
		return fog_table[idx_l];

	u8 y0 = fog_table[idx_l];
	u8 y1 = fog_table[idx_r];
	s32 pos = z - (fog_offset + fog_step * (idx_l + 1));

	return y0 + (y1 - y0) * pos / fog_step;
}

static void
apply_antialiasing(gpu_3d_engine *gpu, s32 y)
{
	for (s32 x = 0; x < 256; x++) {
		auto attr = gpu->attr_buf[0][y][x];

		if (attr.translucent || !attr.edge)
			continue;

		auto& c0 = gpu->color_buf[0][y][x];
		auto& c1 = gpu->color_buf[1][y][x];
		u32 t = attr.coverage;

		c0.a = ((t + 1) * c0.a + (31 - t) * c1.a) >> 5;
		if (c1.a != 0) {
			c0.r = ((t + 1) * c0.r + (31 - t) * c1.r) >> 5;
			c0.g = ((t + 1) * c0.g + (31 - t) * c1.g) >> 5;
			c0.b = ((t + 1) * c0.b + (31 - t) * c1.b) >> 5;
		}
	}
}

} // namespace twice
