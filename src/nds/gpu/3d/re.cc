#include "nds/nds.h"

namespace twice {

using vertex = gpu_3d_engine::vertex;
using polygon = gpu_3d_engine::polygon;
using polygon_info = gpu_3d_engine::rendering_engine::polygon_info;
using slope = gpu_3d_engine::rendering_engine::slope;

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
setup_polygon_slope(slope *s, vertex *v0, vertex *v1)
{
	s32 x0 = v0->sx;
	s32 y0 = v0->sy;
	s32 x1 = v1->sx;
	s32 y1 = v1->sy;

	s->x0 = x0 << 18;
	s->y0 = y0;

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
}

static void
setup_polygon_slope_vertical(slope *s, vertex *v)
{
	vertex v1;
	v1.sx = v->sx;
	v1.sy = v->sy + 1;
	setup_polygon_slope(s, v, &v1);
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
			p->vertices[next_l]);
	pinfo->left = next_l;

	setup_polygon_slope(&pinfo->right_slope, p->vertices[pinfo->start],
			p->vertices[next_r]);
	pinfo->right = next_r;
}

static void
setup_polygon_scanline(gpu_3d_engine *gpu, s32 scanline, u32 poly_num)
{
	polygon *p = gpu->re.polygons[poly_num];
	polygon_info *pinfo = &gpu->re.poly_info[poly_num];

	if (p->vertices[pinfo->start]->sy == p->vertices[pinfo->end]->sy) {
		setup_polygon_slope_vertical(
				&pinfo->left_slope, p->vertices[pinfo->start]);
		setup_polygon_slope_vertical(
				&pinfo->right_slope, p->vertices[pinfo->end]);
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
						p->vertices[next]);
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
						p->vertices[next]);
				pinfo->right = next;
			}
		}
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

	s32 draw_x = std::max(0, xstart[0]);
	s32 draw_x_end = std::min(256, xstart[1]);
	for (s32 x = draw_x; x < draw_x_end; x++) {
		gpu->color_buf[scanline][x] = 0x7FFFC0;
	}

	for (s32 x = xstart[1]; x < xend[0]; x++) {
		if (x < 0 || x >= 256)
			continue;
		gpu->color_buf[scanline][x] = 0x7C0FFF;
	}

	draw_x = std::max(0, xend[0]);
	draw_x_end = std::min(256, xend[1]);
	for (s32 x = draw_x; x < draw_x_end; x++) {
		gpu->color_buf[scanline][x] = 0x7FF03F;
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

	auto& vr = gpu->vtx_ram[gpu->re_buf];

	for (u32 i = 0; i < vr.count; i++) {
		vertex *v = &vr.vertices[i];

		if (v->sx < 0 || v->sx >= 256)
			continue;

		if (v->sy == (s32)scanline) {
			gpu->color_buf[v->sy][v->sx] =
					color6_to_abgr666(v->color);
		}
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
