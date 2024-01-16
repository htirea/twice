#include "nds/gpu/3d/re_poly.h"

#include "nds/nds.h"

namespace twice {

static void find_polygon_start_end_sortkey(polygon *p, bool manual_sort);
static void setup_polygon(rendering_engine *re, re_polygon *p);

void
re_setup_polygons(rendering_engine *re)
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
	s32 start_x = p->vtxs[0]->sx;
	s32 start_y = p->vtxs[0]->sy;

	u32 end_vtx = 0;
	s32 end_x = p->vtxs[0]->sx;
	s32 end_y = p->vtxs[0]->sy;

	for (u32 i = 1; i < p->num_vtxs; i++) {
		vertex *v = p->vtxs[i];
		if (v->sy < start_y || (v->sy == start_y && v->sx < start_x)) {
			start_vtx = i;
			start_x = v->sx;
			start_y = v->sy;
		}
		if (v->sy > end_y || (v->sy == end_y && v->sx >= end_x)) {
			end_vtx = i;
			end_x = v->sx;
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
setup_polygon(rendering_engine *re, re_polygon *p)
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

} // namespace twice
