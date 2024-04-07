#include "nds/gpu/3d/re.h"

#include "common/macros.h"
#include "nds/mem/vram.h"
#include "nds/nds.h"

namespace twice {

struct poly_slope_data {
	slope *sl;
	slope *sr;
	s32 cov_l_start, cov_l_end;
	s32 cov_r_start, cov_r_end;
	s32 x_l_start, x_l_end;
	s32 x_m_start, x_m_end;
	s32 x_r_start, x_r_end;
	s32 i_x0, i_x1;
	bool swapped;
};

struct poly_render_data {
	re_polygon *p;
	bool wireframe;
	bool translucent;
	bool alpha_test;
	bool alpha_blending;
	bool antialiasing;
	bool edge_marking;
	u8 alpha;
	u8 alpha_test_ref;
	s32 cov_y0;
	s32 cov_y1;
	s32 cov_x0;
	s32 cov_x1;
};

/* setup */
static void clear_buffers(rendering_engine *re);
static void setup_polygons(rendering_engine *re);
static void find_polygon_start_end_sortkey(polygon *p, bool manual_sort);
static void setup_polygon(rendering_engine *re, re_polygon *p);
static void setup_polygon_scanline(rendering_engine *re, re_polygon *p, s32 y);
static void setup_polygon_slope(slope *s, polygon *p, u32 start_idx,
		u32 end_idx, bool left, s32 y);
static void setup_poly_slope_data(
		rendering_engine *re, re_polygon *p, s32 y, poly_slope_data&);
static std::pair<bool, bool> setup_poly_fill_rules(rendering_engine *re,
		poly_slope_data&, poly_render_data&, s32 y);
static void get_slope_x_cov(slope *s, s32& x_start, s32& x_end, s32& cov_start,
		s32& cov_end, s32 y);

/* rendering */
static void render_scanline(rendering_engine *re, s32 y);
static void render_polygon_scanline(
		rendering_engine *re, re_polygon *p, s32 y);
static void render_shadow_mask_polygon_pixel(rendering_engine *re,
		poly_render_data& r_data, interpolator *span, u32 x, s32 y,
		u32 attr);
static void render_normal_polygon_pixel(rendering_engine *re,
		poly_render_data& r_data, interpolator *span, u32 x, s32 y,
		u32 attr);
static bool depth_test(rendering_engine *re, s32 z, u32 x, u32 y, bool layer,
		u32 attr, bool wbuffering);
static color4 get_pixel_color(rendering_engine *re, polygon *p, u8 rv, u8 gv,
		u8 bv, u8 av, s32 s, s32 t);
static color4 get_texture_color(
		rendering_engine *re, polygon *p, s32 s, s32 t);
static s32 clamp_or_repeat_texcoords(s32 s, s32 size, bool clamp, bool flip);
static void draw_opaque_pixel(rendering_engine *re, poly_render_data& r_data,
		color4 color, u32 x, u32 y, s32 z, bool layer, u32 attr);
static void draw_translucent_pixel(rendering_engine *re,
		poly_render_data& r_data, color4 color, u32 x, u32 y, s32 z,
		bool layer, bool shadow, u32 attr);

/* effects */
static void apply_effects_scanline(rendering_engine *re, s32 y);
static void apply_edge_marking(rendering_engine *re, s32 y);
static void get_surrounding_id_depth(rendering_engine *re, s32 y, s32 x,
		u32 *id_out, s32 *depth_out);
static void apply_fog(rendering_engine *re, s32 y);
static u8 calculate_fog_density(
		u8 *fog_table, s32 fog_offset, s32 fog_step, s32 z);
static void apply_antialiasing(rendering_engine *re, s32 y);

/* interpolation */
static void interp_setup(interpolator *i, s32 x0, s32 x1, s32 w0, s32 w1,
		bool edge, bool wbuferring);
static void interp_init_attr(interpolator *i, u32 k, s32 y0, s32 y1);
static void interp_update_perspective_factor(interpolator *i);
static void interp_set_attr_perspective(interpolator *i, u32 k);
static void interp_set_attr_linear(interpolator *i, u32 k);
static void interp_update_attr_linear(interpolator *i, u32 k);
static void interp_set_x(interpolator *i, s32 x, u32 k_start, u32 k_end);
static void interp_update_or_set_z_attrs(
		interpolator *i, s32 x, u32 k_start, u32 k_end);
static void interp_update_attrs(
		interpolator *i, s32 x, u32 k_start, u32 k_end);

void
re_render_frame(rendering_engine *re)
{
	if (!re->enabled)
		return;

	setup_fast_texture_vram(re->gpu->nds);
	clear_buffers(re);
	setup_polygons(re);

	for (s32 i = 0; i < 192; i++) {
		render_scanline(re, i);
	}

	for (s32 i = 0; i < 192; i++) {
		apply_effects_scanline(re, i);
	}
}

static void
clear_buffers(rendering_engine *re)
{
	nds_ctx *nds = re->gpu->nds;
	s32 default_depth = 0x200 * re->r.clear_depth + 0x1FF;
	u32 default_attr = (re->r.clear_color & 0x3F008000) | BIT(1);

	if (re->r.disp3dcnt & BIT(14)) {
		u8 x_offset = re->r.clrimage_offset;
		u8 y_offset = re->r.clrimage_offset >> 8;
		u32 rear_attr = re->r.clear_color & 0x3F000000;

		for (u32 y = 0; y < 192; y++) {
			for (u32 x = 0; x < 256; x++) {
				u32 bmp_x = (x_offset + x) & 0xFF;
				u32 bmp_y = (y_offset + y) & 0xFF;
				u32 offset = (bmp_y << 9) + (bmp_x << 1);
				u16 rear_color = vram_read_texture<u16>(
						nds, 0x40000 + offset);
				u32 rear_depth = vram_read_texture<u16>(
						nds, 0x60000 + offset);
				color4 color = unpack_abgr1555_3d(rear_color);
				s32 depth = 0x200 * (rear_depth & 0x7FFF) +
				            0x1FF;
				u32 attr = rear_attr;
				if (color.a != 0) {
					attr |= BIT(1);
				} else {
					/* TODO: check how opaque / transparent
					 * IDs work*/
					attr |= attr >> 8;
				}
				attr |= rear_depth & 0x8000;

				for (u32 l = 0; l < 2; l++) {
					re->color_buf[l][y][x] = color;
					re->depth_buf[l][y][x] = depth;
					re->attr_buf[l][y][x] = attr;
				}
			}
		}

		for (auto& line : re->stencil_buf)
			line.fill(0);

	} else {
		color4 color = unpack_bgr555_3d(re->r.clear_color);
		color.a = re->r.clear_color >> 16 & 0x1F;

		for (u32 l = 0; l < 2; l++) {
			for (auto& line : re->color_buf[l])
				line.fill(color);

			for (auto& line : re->depth_buf[l])
				line.fill(default_depth);

			for (auto& line : re->attr_buf[l])
				line.fill(default_attr);
		}

		for (auto& line : re->stencil_buf)
			line.fill(0);
	}

	re->outside_opaque_id_attr = default_attr & 0x3F000000;
	re->outside_depth = default_depth;
}

static void
setup_polygons(rendering_engine *re)
{
	polygon_ram *pr = re->poly_ram;
	u32 num_polys = pr->count;

	for (u32 i = 0; i < num_polys; i++) {
		re->polys[i].p = &pr->polys[i];
		find_polygon_start_end_sortkey(
				re->polys[i].p, re->manual_sort);
	}

	std::stable_sort(re->polys.begin(), re->polys.begin() + num_polys,
			[](const re_polygon& a, const re_polygon& b) {
				return a.p->sortkey < b.p->sortkey;
			});

	for (u32 i = 0; i < num_polys; i++) {
		setup_polygon(re, &re->polys[i]);
	}
}

static void
find_polygon_start_end_sortkey(polygon *p, bool manual_sort)
{
	u32 start_vtx = 0;
	s32 start_y = p->vtxs[0]->sy;

	u32 end_vtx = 0;
	s32 end_y = p->vtxs[0]->sy;

	for (u32 i = 1; i < p->num_vtxs; i++) {
		vertex *v = p->vtxs[i];
		if (v->sy < start_y) {
			start_vtx = i;
			start_y = v->sy;
		}
		if (v->sy > end_y) {
			end_vtx = i;
			end_y = v->sy;
		}
	}

	p->start_vtx = start_vtx;
	p->end_vtx = end_vtx;

	if (p->translucent) {
		p->sortkey = { 1, manual_sort ? 0 : start_y };
	} else {
		p->sortkey = { 0, start_y };
	}
}

static void
setup_polygon(rendering_engine *, re_polygon *p)
{
	polygon *pp = p->p;
	s32 start_y = pp->vtxs[pp->start_vtx]->sy;
	s32 end_y = pp->vtxs[pp->end_vtx]->sy;

	if (start_y == end_y) {
		end_y++;

		u32 left = pp->num_vtxs - 1;
		s32 left_x = pp->vtxs[left]->sx;
		u32 right = left;
		s32 right_x = left_x;

		for (u32 i = 0; i < 2 && i < pp->num_vtxs; i++) {
			s32 x = pp->vtxs[i]->sx;
			if (x < left_x) {
				left_x = x;
				left = i;
			}
			if (x > right_x) {
				right_x = x;
				right = i;
			}
		}

		p->l = left;
		p->prev_l = left;
		p->r = right;
		p->prev_r = right;
		p->horizontal_line = true;
	} else {
		p->l = pp->start_vtx;
		p->prev_l = pp->start_vtx;
		p->r = pp->start_vtx;
		p->prev_r = pp->start_vtx;
		p->horizontal_line = false;
	}

	p->start_y = start_y;
	p->end_y = end_y;

	s32 top_count = 0;
	s32 bottom_count = 0;
	for (u32 i = 0; i < pp->num_vtxs; i++) {
		vertex *v = pp->vtxs[i];

		if (v->sy == start_y) {
			top_count++;
		}

		if (v->sy == end_y) {
			bottom_count++;
		}
	}

	p->top_edge_y = top_count > 1 ? start_y : -1;
	p->bottom_edge_y = bottom_count > 1 ? end_y - 1 : -1;
	p->id = pp->attr >> 24 & 0x3F;
	p->shadow = (pp->attr >> 4 & 3) == 3;
}

static void
setup_polygon_scanline(rendering_engine *, re_polygon *p, s32 y)
{
	polygon *pp = p->p;
	slope *sl = &p->sl;
	slope *sr = &p->sr;

	if (p->horizontal_line) {
		setup_polygon_slope(sl, pp, p->l, p->l, true, y);
		setup_polygon_slope(sr, pp, p->r, p->r, false, y);
		return;
	}

	u32 curr, next;

	if (pp->vtxs[p->l]->sy == y) {
		curr = p->l;
		next = p->l;

		while (next != pp->end_vtx && pp->vtxs[next]->sy <= y) {
			curr = next;

			next += pp->backface ? -1 : 1;
			if (next == pp->num_vtxs) {
				next = 0;
			}
			if (next == (u32)-1) {
				next = pp->num_vtxs - 1;
			}
		}

		if (next != curr) {
			setup_polygon_slope(sl, pp, curr, next, true, y);
			p->prev_l = curr;
			p->l = next;
		}
	}

	if (pp->vtxs[p->r]->sy == y) {
		curr = p->r;
		next = p->r;

		while (next != pp->end_vtx && pp->vtxs[next]->sy <= y) {
			curr = next;

			next += pp->backface ? 1 : -1;
			if (next == pp->num_vtxs) {
				next = 0;
			}
			if (next == (u32)-1) {
				next = pp->num_vtxs - 1;
			}
		}

		if (next != curr) {
			setup_polygon_slope(sr, pp, curr, next, false, y);
			p->prev_r = curr;
			p->r = next;
		}
	}

	sl->line = sr->line =
			sl->v0->sx == sr->v0->sx && sl->v0->sy == sr->v0->sy &&
			sl->v1->sx == sr->v1->sx && sl->v1->sy == sr->v1->sy;
}

static void
setup_polygon_slope(slope *s, polygon *p, u32 start_idx, u32 end_idx,
		bool left, s32 y)
{
	vertex *v0 = p->vtxs[start_idx];
	vertex *v1 = p->vtxs[end_idx];

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
		s->x0 += s->negative ? -half : half;
	}

	if (s->dx == 0 || s->dy == 0) {
		s->m = 0;
	} else {
		s->m = s->dx * (one / s->dy);
	}

	s->vertical = s->m == 0;
	s->left = left;

	s32 i_x0 = v0->sy;
	s32 i_x1 = v1->sy;
	if (s->dx >= s->dy && x0 != x1 && s->left == s->negative) {
		i_x0--;
		i_x1--;
	}

	interp_setup(&s->i, i_x0, i_x1, p->w[start_idx], p->w[end_idx], true,
			p->wbuffering);

	for (u32 k = 0; k < 5; k++) {
		interp_init_attr(&s->i, k, v0->attr[k], v1->attr[k]);
	}
	interp_init_attr(&s->i, 5, p->w[start_idx], p->w[end_idx]);
	interp_init_attr(&s->i, 6, p->z[start_idx], p->z[end_idx]);
	interp_set_x(&s->i, y, 0, 6);
}

static void
setup_poly_slope_data(
		rendering_engine *, re_polygon *p, s32 y, poly_slope_data& ret)
{
	slope *sl = &p->sl;
	slope *sr = &p->sr;

	get_slope_x_cov(sl, ret.x_l_start, ret.x_l_end, ret.cov_l_start,
			ret.cov_l_end, y);
	get_slope_x_cov(sr, ret.x_r_start, ret.x_r_end, ret.cov_r_start,
			ret.cov_r_end, y);

	if (sr->vertical && ret.x_r_start != 0 &&
			!(sl->vertical && ret.x_l_start == ret.x_r_start)) {
		ret.x_r_start--;
		ret.x_r_end--;
	}

	if (ret.x_l_start >= ret.x_r_end) {
		ret.swapped = true;
		std::swap(sl, sr);
		std::swap(ret.x_l_start, ret.x_r_start);
		std::swap(ret.x_l_end, ret.x_r_end);
		std::swap(ret.cov_l_start, ret.cov_r_start);
		std::swap(ret.cov_l_end, ret.cov_r_end);

		/* TODO: break antialiasing */
		ret.cov_l_start ^= 0x3FF;
		ret.cov_l_end ^= 0x3FF;
		ret.cov_r_start ^= 0x3FF;
		ret.cov_r_end ^= 0x3FF;
	} else {
		ret.swapped = false;
	}

	ret.x_r_start = std::max(ret.x_r_start, ret.x_l_end);
	ret.x_m_start = ret.x_l_end;
	ret.x_m_end = ret.x_r_start;
	ret.i_x0 = ret.x_l_start;
	ret.i_x1 = ret.x_r_end;

	if (ret.swapped) {
		if (sl->xmajor) {
			ret.x_l_start = ret.x_l_end - 1;
		}
		if (sr->xmajor) {
			ret.x_r_end = ret.x_r_start + 1;
		}
	}

	ret.sl = sl;
	ret.sr = sr;
}

static std::pair<bool, bool>
setup_poly_fill_rules(rendering_engine *, poly_slope_data& s_data,
		poly_render_data& r_data, s32 y)
{
	bool fill_left, fill_right;
	slope *sl = s_data.sl;
	slope *sr = s_data.sr;
	re_polygon *p = r_data.p;

	bool bottom_edge_fill_rule =
			y == p->bottom_edge_y && sl->v1->sx != sr->v1->sx;
	bool default_left_fill_rule = sl->negative || !sl->xmajor ||
	                              sl->line ||
	                              (sl->xmajor && bottom_edge_fill_rule);
	bool default_right_fill_rule = (!sr->negative && sr->xmajor) ||
	                               (sr->xmajor && bottom_edge_fill_rule);
	bool force_fill_edge = r_data.wireframe ||
	                       (r_data.translucent && r_data.alpha_blending) ||
	                       r_data.antialiasing || r_data.edge_marking;
	if (!s_data.swapped) {
		fill_left = default_left_fill_rule;

		if (sr->vertical)
			fill_right = true;
		else
			fill_right = default_right_fill_rule;
	} else {
		if (sl->vertical)
			fill_left = true;
		else
			fill_left = default_left_fill_rule;

		if (sl->vertical)
			fill_right = !(sr->negative && sr->xmajor);
		else if (sr->vertical)
			fill_right = false;
		else
			fill_right = default_right_fill_rule;
	}

	fill_left = fill_left || force_fill_edge;
	fill_right = fill_right || force_fill_edge;

	return { fill_left, fill_right };
}

static void
get_slope_x_cov(slope *s, s32& x_start, s32& x_end, s32& cov_start,
		s32& cov_end, s32 y)
{
	s32 one = (s32)1 << 18;
	s32 dy = y - s->y0;

	if (s->xmajor && !s->negative) {
		x_start = s->x0 + dy * s->m;
		x_end = (x_start & ~0x1FF) + s->m - one;
	} else if (s->xmajor) {
		x_end = s->x0 - dy * s->m;
		x_start = (x_end | 0x1FF) - s->m + one;
	} else if (!s->negative) {
		x_start = s->x0 + dy * s->m;
		x_end = x_start;
	} else {
		x_start = s->x0 - dy * s->m;
		x_end = x_start;
	}

	if (s->vertical || s->dy == 0) {
		cov_start = 0x3FF;
		cov_end = 0x3FF;
	} else if (s->dx == s->dy) {
		cov_start = 0x1FF;
		cov_end = 0x1FF;
	} else if (s->xmajor) {
		s32 x0 = x_start >> 18;
		s32 x1 = x_end >> 18;

		cov_start = (x0 - s->v0->sx) * (s->dy << 10) / s->dx & 0x3FF;
		cov_end = (x1 - s->v0->sx) * (s->dy << 10) / s->dx & 0x3FF;

		if (s->wide) {
			if (cov_start >= 0x200 && x0 != x1) {
				cov_start ^= 0x3FF;
			}
		} else {
			cov_start = (cov_start + 0x200) & 0x3FF;
			if (x0 != x1) {
				cov_end = 0x3FF;
			}
		}

		if (!s->left) {
			cov_start ^= 0x3FF;
			cov_end ^= 0x3FF;
		}
	} else {
		cov_start = x_start >> 8 & 0x3FF;
		cov_end = cov_start;

		if (s->left) {
			cov_start ^= 0x3FF;
			cov_end ^= 0x3FF;
		}
	}

	x_start >>= 18;
	x_end >>= 18;
	x_end++;
}

static void
render_scanline(rendering_engine *re, s32 y)
{
	u32 num_polys = re->poly_ram->count;

	for (u32 i = 0; i < num_polys; i++) {
		re_polygon *p = &re->polys[i];
		if (y < p->start_y || y >= p->end_y)
			continue;

		render_polygon_scanline(re, p, y);
	}
}

static void
render_polygon_scanline(rendering_engine *re, re_polygon *p, s32 y)
{
	poly_slope_data s_data;
	poly_render_data r_data;
	polygon *pp = p->p;

	setup_polygon_scanline(re, p, y);

	bool shadow_mask = p->shadow && p->id == 0;
	if (shadow_mask && !re->last_poly_is_shadow_mask) {
		re->stencil_buf[y].fill(0);
	}
	re->last_poly_is_shadow_mask = shadow_mask;

	setup_poly_slope_data(re, p, y, s_data);
	slope *sl = s_data.sl;
	slope *sr = s_data.sr;

	r_data.p = p;
	u32 attr_alpha = pp->attr >> 16 & 0x1F;
	r_data.wireframe = attr_alpha == 0;
	r_data.translucent = attr_alpha != 0 && attr_alpha != 31;
	r_data.alpha_test = re->r.disp3dcnt & BIT(2);
	r_data.alpha_blending = re->r.disp3dcnt & BIT(3);
	r_data.antialiasing = re->r.disp3dcnt & BIT(4);
	r_data.edge_marking = re->r.disp3dcnt & BIT(5);
	r_data.alpha = attr_alpha == 0 ? 31 : attr_alpha;
	r_data.alpha_test_ref = r_data.alpha_test ? re->r.alpha_test_ref : 0;

	bool top_or_bottom_edge = y == p->top_edge_y || y == p->bottom_edge_y;
	bool fill_middle = !r_data.wireframe || top_or_bottom_edge;
	auto [fill_left, fill_right] =
			setup_poly_fill_rules(re, s_data, r_data, y);

	interp_update_or_set_z_attrs(&sl->i, y, shadow_mask ? 5 : 0, 6);
	interp_update_or_set_z_attrs(&sr->i, y, shadow_mask ? 5 : 0, 6);
	interpolator span;
	interp_setup(&span, s_data.i_x0, s_data.i_x1, sl->i.attrs[5].y,
			sr->i.attrs[5].y, false, pp->wbuffering);
	if (!shadow_mask) {
		for (u32 k = 0; k < 5; k++) {
			interp_init_attr(&span, k, sl->i.attrs[k].y,
					sr->i.attrs[k].y);
		}
	}
	interp_init_attr(&span, 6, sl->i.attrs[6].y, sr->i.attrs[6].y);

	u32 base_attr = (pp->attr & 0x3F00C800) | (u32)pp->backface << 7;

	if (fill_left) {
		u32 start = std::max(0, s_data.x_l_start);
		u32 end = std::min({ 256, s_data.x_l_end, s_data.x_r_end });
		u32 attr = base_attr | 1;

		if (shadow_mask) {
			for (u32 x = start; x < end; x++) {
				render_shadow_mask_polygon_pixel(
						re, r_data, &span, x, y, attr);
			}
		} else {
			if (r_data.antialiasing) {
				r_data.cov_y0 = s_data.cov_l_start;
				r_data.cov_y1 = s_data.cov_l_end;
				r_data.cov_x0 = s_data.x_l_start;
				r_data.cov_x1 = s_data.x_l_end - 1;
			}

			for (u32 x = start; x < end; x++) {
				render_normal_polygon_pixel(
						re, r_data, &span, x, y, attr);
			}
		}
	}

	if (fill_middle) {
		u32 start = std::max(0, s_data.x_m_start);
		u32 end = std::min(256, s_data.x_m_end);
		u32 attr = base_attr | top_or_bottom_edge;

		if (shadow_mask) {
			for (u32 x = start; x < end; x++) {
				render_shadow_mask_polygon_pixel(
						re, r_data, &span, x, y, attr);
			}
		} else {
			if (r_data.antialiasing) {
				r_data.cov_y0 = 0x3FF;
				r_data.cov_y1 = 0x3FF;
				r_data.cov_x0 = 0;
				r_data.cov_x1 = 0;
			}

			for (u32 x = start; x < end; x++) {
				render_normal_polygon_pixel(
						re, r_data, &span, x, y, attr);
			}
		}
	}

	if (fill_right) {
		u32 start = std::max(0, s_data.x_r_start);
		u32 end = std::min(256, s_data.x_r_end);
		u32 attr = base_attr | 1;

		if (shadow_mask) {
			for (u32 x = start; x < end; x++) {
				render_shadow_mask_polygon_pixel(
						re, r_data, &span, x, y, attr);
			}
		} else {
			if (r_data.antialiasing) {
				r_data.cov_y0 = s_data.cov_r_start;
				r_data.cov_y1 = s_data.cov_r_end;
				r_data.cov_x0 = s_data.x_r_start;
				r_data.cov_x1 = s_data.x_r_end - 1;
			}

			for (u32 x = start; x < end; x++) {
				render_normal_polygon_pixel(
						re, r_data, &span, x, y, attr);
			}
		}
	}
}

static void
render_shadow_mask_polygon_pixel(rendering_engine *re,
		poly_render_data& r_data, interpolator *span, u32 x, s32 y,
		u32 attr)
{
	interp_update_or_set_z_attrs(span, x, 0, 0);
	s32 z = span->attrs[6].y;

	if (r_data.alpha <= r_data.alpha_test_ref)
		return;

	bool wbuffering = r_data.p->p->wbuffering;

	if (!depth_test(re, z, x, y, 0, attr, wbuffering)) {
		re->stencil_buf[y][x] = 1;
	}

	if (re->attr_buf[0][y][x] & 1) {
		if (!depth_test(re, z, x, y, 1, attr, wbuffering)) {
			re->stencil_buf[y][x] |= 2;
		}
	}
}

static void
render_normal_polygon_pixel(rendering_engine *re, poly_render_data& r_data,
		interpolator *span, u32 x, s32 y, u32 attr)
{
	re_polygon *p = r_data.p;
	bool layer = 0;
	bool shadow = p->shadow;

	if (shadow) {
		u8 stencil = re->stencil_buf[y][x];
		if (stencil == 0)
			return;

		if (!(stencil & 1)) {
			layer = 1;
		}
	}

	interp_update_or_set_z_attrs(span, x, 0, 0);
	s32 z = span->attrs[6].y;
	if (!depth_test(re, z, x, y, layer, attr, p->p->wbuffering)) {
		if (layer == 1 || !(re->attr_buf[0][y][x] & 1))
			return;

		layer = 1;

		if (shadow && !(re->stencil_buf[y][x] & 2))
			return;

		if (!depth_test(re, z, x, y, 1, attr, p->p->wbuffering))
			return;
	}

	interp_update_attrs(span, x, 0, 5);
	color4 px_color = get_pixel_color(re, p->p, span->attrs[0].y >> 3,
			span->attrs[1].y >> 3, span->attrs[2].y >> 3,
			r_data.alpha, span->attrs[3].y, span->attrs[4].y);
	if (px_color.a <= r_data.alpha_test_ref)
		return;

	if (px_color.a == 31) {
		if ((attr & 1) && r_data.antialiasing && layer == 0) {
			s32 cov = ilerp(r_data.cov_y0, r_data.cov_y1,
					r_data.cov_x0, r_data.cov_x1, x);
			attr = (attr & ~0x1F00) | (cov >> 5 << 8);
		}

		if (layer == 0) {
			re->color_buf[1][y][x] = re->color_buf[0][y][x];
			re->depth_buf[1][y][x] = re->depth_buf[0][y][x];
			re->attr_buf[1][y][x] = re->attr_buf[0][y][x];
		}

		draw_opaque_pixel(re, r_data, px_color, x, y, z, layer, attr);
	} else {
		draw_translucent_pixel(re, r_data, px_color, x, y, z, layer,
				shadow, attr);
		if (layer == 0) {
			draw_translucent_pixel(re, r_data, px_color, x, y, z,
					1, shadow, attr);
		}
	}
}

static bool
depth_test(rendering_engine *re, s32 z, u32 x, u32 y, bool layer, u32 attr,
		bool wbuffering)
{
	s32 z_dest = re->depth_buf[layer][y][x];
	u32 attr_dest = re->attr_buf[layer][y][x];

	if (attr & BIT(14)) {
		s32 margin = wbuffering ? 0xFF : 0x200;
		return z <= z_dest + margin;
	}

	/* TODO: wireframe edge */

	if (!(attr & BIT(7)) && (attr_dest & BIT(1) && attr_dest & BIT(7))) {
		return z <= z_dest;
	}

	return z < z_dest;
}

static color4
get_pixel_color(rendering_engine *re, polygon *p, u8 rv, u8 gv, u8 bv, u8 av,
		s32 s, s32 t)
{
	u32 tx_format = p->tx_param >> 26 & 7;
	u32 blend_mode = p->attr >> 4 & 3;

	u8 rs, gs, bs;
	bool highlight_shading = blend_mode == 2 && re->r.disp3dcnt & 2;
	if (blend_mode == 2) {
		u16 toon_color = re->r._toon_table[rv >> 1];
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
	if (tx_format == 0 || !(re->r.disp3dcnt & 1)) {
		r = rv;
		g = gv;
		b = bv;
		a = av;
	} else {
		auto [rt, gt, bt, at] = get_texture_color(re, p, s, t);

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
get_texture_color(rendering_engine *re, polygon *p, s32 s, s32 t)
{
	u8 r{}, g{}, b{}, a{};
	nds_ctx *nds = re->gpu->nds;
	u32 tx_format = p->tx_param >> 26 & 7;
	u32 base_offset = (p->tx_param & 0xFFFF) << 3;
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

	switch (tx_format) {
	case 2:
	{
		u32 offset = base_offset;
		offset += (t >> 4) * (s_size >> 2);
		offset += (s >> 4) >> 2;
		u8 color_num = vram_read_texture<u8>(nds, offset);
		color_num = color_num >> ((s >> 4 & 3) << 1) & 3;
		if (color_num != 0 || !color_0_transparent) {
			u32 palette_offset =
					(p->pltt_base << 3) + (color_num << 1);
			u16 color = vram_read_texture_palette<u16>(
					nds, palette_offset);
			unpack_bgr555_3d(color, &r, &g, &b);
			a = 31;
		}
		break;
	}
	case 3:
	{
		u32 offset = base_offset;
		offset += (t >> 4) * (s_size >> 1);
		offset += (s >> 4) >> 1;
		u8 color_num = vram_read_texture<u8>(nds, offset);
		color_num = color_num >> ((s >> 4 & 1) << 2) & 0xF;
		if (color_num != 0 || !color_0_transparent) {
			u32 palette_offset =
					(p->pltt_base << 4) + (color_num << 1);
			u16 color = vram_read_texture_palette<u16>(
					nds, palette_offset);
			unpack_bgr555_3d(color, &r, &g, &b);
			a = 31;
		}
		break;
	}
	case 4:
	{
		u32 offset = base_offset + (t >> 4) * s_size + (s >> 4);
		u8 color_num = vram_read_texture<u8>(nds, offset);
		if (color_num != 0 || !color_0_transparent) {
			u32 palette_offset =
					(p->pltt_base << 4) + (color_num << 1);
			u16 color = vram_read_texture_palette<u16>(
					nds, palette_offset);
			unpack_bgr555_3d(color, &r, &g, &b);
			a = 31;
		}
		break;
	}
	case 1:
	{
		u32 offset = base_offset + (t >> 4) * s_size + (s >> 4);
		u8 data = vram_read_texture<u8>(nds, offset);
		u8 color_num = data & 0x1F;
		u8 alpha = data >> 5;
		u32 palette_offset = (p->pltt_base << 4) + (color_num << 1);
		u16 color = vram_read_texture_palette<u16>(
				nds, palette_offset);
		unpack_bgr555_3d(color, &r, &g, &b);
		a = (alpha << 2) + (alpha >> 1);
		break;
	}
	case 6:
	{
		u32 offset = base_offset + (t >> 4) * s_size + (s >> 4);
		u8 data = vram_read_texture<u8>(nds, offset);
		u8 color_num = data & 7;
		u32 palette_offset = (p->pltt_base << 4) + (color_num << 1);
		u16 color = vram_read_texture_palette<u16>(
				nds, palette_offset);
		unpack_bgr555_3d(color, &r, &g, &b);
		a = data >> 3;
		break;
	}
	case 7:
	{
		u32 offset = base_offset;
		offset += (t >> 4) * s_size << 1;
		offset += (s >> 4) << 1;
		u16 color = vram_read_texture<u16>(nds, offset);
		unpack_abgr1555_3d(color, &r, &g, &b, &a);
		break;
	}
	case 5:
	{
		u32 offset = base_offset;
		offset += (t >> 4 & ~3) * (s_size >> 2);
		offset += (s >> 4 & ~3);
		u32 data = vram_read_texture<u32>(nds, offset);
		u32 color_num = data >> ((t >> 4 & 3) << 3) >>
		                ((s >> 4 & 3) << 1);
		color_num &= 3;
		u32 index_offset = 0x20000 + ((offset & 0x1FFFF) >> 1);
		if (offset >= 0x40000) {
			index_offset += 0x10000;
		}
		u16 index_data = vram_read_texture<u32>(nds, index_offset);
		u32 palette_base = (p->pltt_base << 4) +
		                   ((index_data & 0x3FFF) << 2);
		u32 palette_mode = index_data >> 14;

		switch (color_num) {
		case 0:
		{
			u16 color = vram_read_texture_palette<u16>(
					nds, palette_base);
			unpack_bgr555_3d(color, &r, &g, &b);
			a = 31;
			break;
		}
		case 1:
		{
			u16 color = vram_read_texture_palette<u16>(
					nds, palette_base + 2);
			unpack_bgr555_3d(color, &r, &g, &b);
			a = 31;
			break;
		}
		case 2:
		{
			switch (palette_mode) {
			case 0:
			case 2:
			{
				u16 color = vram_read_texture_palette<u16>(
						nds, palette_base + 4);
				unpack_bgr555_3d(color, &r, &g, &b);
				a = 31;
				break;
			}
			case 1:
			case 3:
			{
				u16 color0 = vram_read_texture_palette<u16>(
						nds, palette_base);
				u16 color1 = vram_read_texture_palette<u16>(
						nds, palette_base + 2);
				if (palette_mode == 1) {
					blend_bgr555_11_3d(color0, color1, &r,
							&g, &b);
				} else {
					blend_bgr555_53_3d(color0, color1, &r,
							&g, &b);
				}
				a = 31;
			}
			}
			break;
		}
		case 3:
		{
			switch (palette_mode) {
			case 0:
			case 1:
				a = 0;
				break;
			case 2:
			{
				u16 color = vram_read_texture_palette<u16>(
						nds, palette_base + 6);
				unpack_bgr555_3d(color, &r, &g, &b);
				a = 31;
				break;
			}
			case 3:

			{
				u16 color0 = vram_read_texture_palette<u16>(
						nds, palette_base);
				u16 color1 = vram_read_texture_palette<u16>(
						nds, palette_base + 2);
				blend_bgr555_53_3d(color1, color0, &r, &g, &b);
				a = 31;
			}
			}
		}
		}
		break;
	}
	}

	return { r, g, b, a };
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
draw_opaque_pixel(rendering_engine *re, poly_render_data&, color4 color, u32 x,
		u32 y, s32 z, bool layer, u32 attr)
{
	re->color_buf[layer][y][x] = color;
	re->depth_buf[layer][y][x] = z;
	u32 attr_dest = re->attr_buf[layer][y][x];
	attr = (attr_dest & 0x3F0000) | (attr & ~0x3F0000) | BIT(1);
	re->attr_buf[layer][y][x] = attr;
}

static void
draw_translucent_pixel(rendering_engine *re, poly_render_data& r_data,
		color4 color, u32 x, u32 y, s32 z, bool layer, bool shadow,
		u32 attr)
{
	u32 attr_dest = re->attr_buf[layer][y][x];
	if (!(attr_dest & BIT(1))) {
		if (attr >> 24 == attr_dest << 8 >> 24)
			return;
	} else if (shadow) {
		if (attr >> 24 == attr_dest >> 24)
			return;
	}

	color4& color_dest = re->color_buf[layer][y][x];
	if (color_dest.a == 0) {
		color_dest = color;
	} else if (!r_data.alpha_blending) {
		auto [r, g, b, a] = color;
		a = std::max(a, color_dest.a);
		color_dest = { r, g, b, std::max(a, color_dest.a) };
	} else {
		auto [r1, g1, b1, a1] = color;
		auto [r2, g2, b2, a2] = color_dest;
		u8 r = ((a1 + 1) * r1 + (31 - a1) * r2) >> 5;
		u8 g = ((a1 + 1) * g1 + (31 - a1) * g2) >> 5;
		u8 b = ((a1 + 1) * b1 + (31 - a1) * b2) >> 5;
		u8 a = std::max(a1, a2);
		color_dest = { r, g, b, a };
	}

	if (attr & BIT(11)) {
		re->depth_buf[layer][y][x] = z;
	}

	attr = (attr_dest & 0x3F000000) | (attr >> 8 & 0x3F0000) |
	       (attr & 0xFFFF);
	attr = (attr & ~BIT(15)) | (attr & attr_dest & BIT(15));
	re->attr_buf[layer][y][x] = attr;
}

static void
apply_effects_scanline(rendering_engine *re, s32 y)
{
	if (re->r.disp3dcnt & BIT(5)) {
		apply_edge_marking(re, y);
	}

	if (re->r.disp3dcnt & BIT(7)) {
		apply_fog(re, y);
	}

	if (re->r.disp3dcnt & BIT(4)) {
		apply_antialiasing(re, y);
	}
}

static void
apply_edge_marking(rendering_engine *re, s32 y)
{
	for (u32 x = 0; x < 256; x++) {
		u32 attr = re->attr_buf[0][y][x];

		if (!(attr & BIT(1)) || !(attr & BIT(0)))
			continue;

		u32 adj_ids[4];
		s32 adj_depths[4];
		get_surrounding_id_depth(re, y, x, adj_ids, adj_depths);

		u32 id = attr & 0x3F000000;
		s32 depth = re->depth_buf[0][y][x];

		bool mark = false;
		for (u32 i = 0; i < 4; i++) {
			if (adj_ids[i] != id && depth < adj_depths[i]) {
				mark = true;
			}
		}

		if (!mark)
			continue;

		u16 color = re->r._edge_color[id >> 24 >> 3];
		auto [r, g, b, _] = unpack_bgr555_3d(color);
		re->color_buf[0][y][x].r = r;
		re->color_buf[0][y][x].g = g;
		re->color_buf[0][y][x].b = b;
		re->attr_buf[0][y][x] = (attr & ~0x1F00) | (u32)0x10 << 8;
	}
}

static void
get_surrounding_id_depth(rendering_engine *re, s32 y, s32 x, u32 *id_out,
		s32 *depth_out)
{
	s32 offsets[4][2] = { { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 } };

	for (u32 i = 0; i < 4; i++) {
		s32 x2 = x + offsets[i][0];
		s32 y2 = y + offsets[i][1];

		if (x2 < 0 || x2 >= 256 || y2 < 0 || y2 >= 192) {
			id_out[i] = re->outside_opaque_id_attr;
			depth_out[i] = re->outside_depth;
		} else {
			id_out[i] = re->attr_buf[0][y2][x2] & 0x3F000000;
			depth_out[i] = re->depth_buf[0][y2][x2];
		}
	}
}

static void
apply_fog(rendering_engine *re, s32 y)
{
	u16 fog_shift = re->r.disp3dcnt >> 8 & 0xF;
	s32 fog_step = 0x400 >> fog_shift;
	u16 fog_offset = re->r.fog_offset;
	bool alpha_only = re->r.disp3dcnt & BIT(6);

	auto [rf, gf, bf, af] = unpack_bgr555_3d(re->r.fog_color);
	af = re->r.fog_color >> 16 & 0x1F;

	for (s32 x = 0; x < 256; x++) {
		for (int layer = 0; layer < 2; layer++) {
			if (!(re->attr_buf[layer][y][x] & BIT(15)))
				continue;

			u32 z = re->depth_buf[layer][y][x] >> 9;
			u8 t = calculate_fog_density(re->r.fog_table.data(),
					fog_offset, fog_step, z);
			if (t >= 127) {
				t = 128;
			}

			auto& dst = re->color_buf[layer][y][x];
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
apply_antialiasing(rendering_engine *re, s32 y)
{
	for (s32 x = 0; x < 256; x++) {
		u32 attr = re->attr_buf[0][y][x];

		if (!(attr & BIT(1)) || !(attr & BIT(0)))
			continue;

		auto& c0 = re->color_buf[0][y][x];
		auto& c1 = re->color_buf[1][y][x];
		u32 t = attr >> 8 & 0x1F;

		c0.a = ((t + 1) * c0.a + (31 - t) * c1.a) >> 5;
		if (c1.a != 0) {
			c0.r = ((t + 1) * c0.r + (31 - t) * c1.r) >> 5;
			c0.g = ((t + 1) * c0.g + (31 - t) * c1.g) >> 5;
			c0.b = ((t + 1) * c0.b + (31 - t) * c1.b) >> 5;
		}
	}
}

static void
interp_setup(interpolator *i, s32 x0, s32 x1, s32 w0, s32 w1, bool edge,
		bool wbuferring)
{
	i->x0 = x0;
	i->x1 = x1;
	i->x_len = x1 - x0;
	i->w0 = w0;
	i->w1 = w1;
	i->w0_n = w0;
	i->w0_d = w0;
	i->w1_d = w1;

	if ((edge && w0 == w1 && !(w0 & 0x7E)) ||
			(!edge && w0 == w1 && !(w0 & 0x7F))) {
		i->precision = 0;
	} else {
		if (edge) {
			if (w0 & ~w1 & 1) {
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

	i->wbuffering = wbuferring;
	i->next_z_x = -1;
	i->next_attr_x = -1;
}

static void
interp_init_attr(interpolator *i, u32 k, s32 y0, s32 y1)
{
	auto& attr = i->attrs[k];

	attr.y0 = y0;
	attr.y1 = y1;
	attr.positive = y0 <= y1;
	attr.y_len = attr.positive ? y1 - y0 : y0 - y1;

	if (i->precision == 0 || !i->wbuffering) {
		if (i->x_len == 0) {
			attr.y_step = 0;
			attr.y_rem_step = 0;
		} else {
			attr.y_step = attr.y_len / i->x_len;
			attr.y_rem_step = attr.y_len % i->x_len;
		}
	}
}

static void
interp_update_perspective_factor(interpolator *i)
{
	i->pfactor = i->denom == 0 ? 0 : i->numer / i->denom;
	i->pfactor_r = ((u32)1 << i->precision) - i->pfactor;
}

static void
interp_set_attr_perspective(interpolator *i, u32 k)
{
	auto& attr = i->attrs[k];

	if (attr.positive) {
		attr.y = attr.y0 + (attr.y_len * i->pfactor >> i->precision);
	} else {
		attr.y = attr.y1 + (attr.y_len * i->pfactor_r >> i->precision);
	}
}

static void
interp_set_attr_linear(interpolator *i, u32 k)
{
	auto& attr = i->attrs[k];

	if (i->x_len == 0) {
		attr.y = attr.y0;
		attr.y_rem = 0;
	} else if (attr.positive) {
		attr.y = attr.y0 + attr.y_len * (i->x - i->x0) / i->x_len;
		attr.y_rem = attr.y_len * (i->x - i->x0) % i->x_len;
	} else {

		attr.y = attr.y1 + attr.y_len * (i->x1 - i->x) / i->x_len;
		attr.y_rem = attr.y_len * (i->x1 - i->x) % i->x_len;
	}
}

static void
interp_update_attr_linear(interpolator *i, u32 k)
{
	auto& attr = i->attrs[k];

	if (attr.positive) {
		attr.y += attr.y_step;
		attr.y_rem += attr.y_rem_step;
		if (attr.y_rem >= i->x_len) {
			attr.y += 1;
			attr.y_rem -= i->x_len;
		}
	} else {
		attr.y -= attr.y_step;
		attr.y_rem -= attr.y_rem_step;
		if (attr.y_rem < 0) {
			attr.y -= 1;
			attr.y_rem += i->x_len;
		}
	}
}

static void
interp_set_x(interpolator *i, s32 x, u32 k_start, u32 k_end)
{
	i->x = x;

	if (i->precision != 0 || i->wbuffering) {
		i->numer = ((s64)i->w0_n << i->precision) * (i->x - i->x0);
		i->denom = ((s64)i->w1_d * (i->x1 - i->x)) +
		           ((s64)i->w0_d * (i->x - i->x0));
		interp_update_perspective_factor(i);
	}

	if (i->wbuffering) {
		interp_set_attr_perspective(i, 6);
	} else {
		interp_set_attr_linear(i, 6);
	}
	i->next_z_x = x + 1;

	if (k_start < k_end) {
		if (i->precision != 0) {
			for (u32 k = k_start; k < k_end; k++) {
				interp_set_attr_perspective(i, k);
			}
		} else {
			for (u32 k = k_start; k < k_end; k++) {
				interp_set_attr_linear(i, k);
			}
		}
		i->next_attr_x = x + 1;
	}
}

static void
interp_update_or_set_z_attrs(interpolator *i, s32 x, u32 k_start, u32 k_end)
{
	if (x != i->next_z_x) {
		interp_set_x(i, x, k_start, k_end);
		return;
	}

	i->x++;

	if (i->precision != 0 || i->wbuffering) {
		i->numer += (s64)i->w0_n << i->precision;
		i->denom += i->w0_d - i->w1_d;
		/* TODO: there is probably a way to incrementally update the
		 * attribute values
		 */
		interp_update_perspective_factor(i);
	}

	if (i->wbuffering) {
		interp_set_attr_perspective(i, 6);
	} else {
		interp_update_attr_linear(i, 6);
	}
	i->next_z_x = x + 1;

	interp_update_attrs(i, x, k_start, k_end);
}

static void
interp_update_attrs(interpolator *i, s32 x, u32 k_start, u32 k_end)
{
	if (i->precision == 0) {
		if (x != i->next_attr_x) {
			for (u32 k = k_start; k < k_end; k++) {
				interp_set_attr_linear(i, k);
			}
		} else {
			for (u32 k = k_start; k < k_end; k++) {
				interp_update_attr_linear(i, k);
			}
		}
	} else {
		for (u32 k = k_start; k < k_end; k++) {
			interp_set_attr_perspective(i, k);
		}
	}
	i->next_attr_x = x + 1;
}

} // namespace twice
