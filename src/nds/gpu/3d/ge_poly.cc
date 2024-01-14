#include "nds/gpu/3d/ge_poly.h"

#include "common/logger.h"
#include "common/util.h"
#include "nds/gpu/3d/ge_matrix.h"
#include "nds/gpu/3d/gpu3d.h"
#include "nds/gpu/3d/interpolator.h"

namespace twice {

static std::vector<vertex> clip_polygon_plane(std::vector<vertex>& in,
		int plane, int positive, bool render_clipped);
static bool vertex_inside(vertex *v, int plane, int positive);
static vertex create_clipped_vertex(
		vertex *v0, vertex *v1, int plane, int positive);
static s32 clip_lerp(s32 y0, s32 y1, interpolator *i, int positive);
static void add_polygon(geometry_engine *ge);
static s64 get_face_direction(vertex *v0, vertex *v1, vertex *v2);
static void texcoord_transform_3(geometry_engine *ge, s64 vx, s64 vy, s64 vz);

void
ge_add_vertex(geometry_engine *ge)
{
	vertex_ram *vr = ge->vtx_ram;

	if (vr->count >= 6144) {
		LOG("vertex ram full\n");
		return;
	}

	vertex *v = &ge->vtx_buf[ge->vtx_count & 3];
	ge_mtx_mult_vec(v->pos, ge->clip_mtx,
			std::array<s32, 4>{ ge->vx, ge->vy, ge->vz, 1 << 12 });

	v->attr[0] = (s32)ge->vtx_color[0] << 3;
	v->attr[1] = (s32)ge->vtx_color[1] << 3;
	v->attr[2] = (s32)ge->vtx_color[2] << 3;

	if (ge->teximage_param >> 30 == 3) {
		texcoord_transform_3(ge, ge->vx, ge->vy, ge->vz);
	}
	v->attr[3] = ge->vtx_texcoord[0];
	v->attr[4] = ge->vtx_texcoord[1];

	ge->vtx_count++;

	switch (ge->primitive_type) {
	case 0:
		if (ge->vtx_count % 3 == 0) {
			add_polygon(ge);
		}
		break;
	case 1:
		if (ge->vtx_count % 4 == 0) {
			add_polygon(ge);
		}
		break;
	case 2:
		if (ge->vtx_count >= 3) {
			add_polygon(ge);
		}
		break;
	case 3:
		if (ge->vtx_count >= 4 && ge->vtx_count % 2 == 0) {
			add_polygon(ge);
		}
	}
}

std::vector<vertex>
ge_clip_polygon(std::span<vertex *> in, u32 num_vertices,
		bool render_clipped_far)
{
	std::vector<vertex> out;
	for (u32 i = 0; i < num_vertices; i++) {
		out.push_back(*in[i]);
	}

	out = clip_polygon_plane(out, 2, 1, render_clipped_far);
	out = clip_polygon_plane(out, 2, 0, true);
	out = clip_polygon_plane(out, 1, 1, true);
	out = clip_polygon_plane(out, 1, 0, true);
	out = clip_polygon_plane(out, 0, 1, true);
	out = clip_polygon_plane(out, 0, 0, true);

	return out;
}

static std::vector<vertex>
clip_polygon_plane(std::vector<vertex>& in, int plane, int positive,
		bool render_clipped)
{
	std::vector<vertex> out;
	size_t n = in.size();

	for (u32 curr_idx = 0; curr_idx < n; curr_idx++) {
		u32 next_idx = (curr_idx + 1) % n;
		vertex *curr = &in[curr_idx];
		vertex *next = &in[next_idx];
		bool curr_inside = vertex_inside(curr, plane, positive);
		bool next_inside = vertex_inside(next, plane, positive);

		if (!(curr_inside && next_inside) && !render_clipped) {
			return {};
		}

		if (curr_inside && next_inside) {
			out.push_back(*curr);
		} else if (curr_inside) {
			out.push_back(*curr);
			out.push_back(create_clipped_vertex(
					curr, next, plane, positive));
		} else if (next_inside) {
			out.push_back(create_clipped_vertex(
					curr, next, plane, positive));
		}
	}

	return out;
}

static bool
vertex_inside(vertex *v, int plane, int positive)
{
	return positive ? v->pos[plane] <= v->pos[3]
	                : v->pos[plane] >= -v->pos[3];
}

static vertex
create_clipped_vertex(vertex *v0, vertex *v1, int plane, int positive)
{
	vertex r;
	interpolator i;
	i.x0 = v0->pos[plane];
	i.x1 = v1->pos[plane];
	i.w0 = v0->pos[3];
	i.w1 = v1->pos[3];
	i.denom = positive ? i.x0 - i.w0 + i.w1 - i.x1
	                   : i.x0 + i.w0 - i.w1 - i.x1;

	if (i.denom == 0) {
		r.pos[3] = i.w1;
	} else {
		r.pos[3] = ((s64)i.w1 * i.x0 - (s64)i.w0 * i.x1) / i.denom;
	}

	switch (plane) {
	case 0:
		r.pos[0] = positive ? r.pos[3] : -r.pos[3];
		r.pos[1] = clip_lerp(v0->pos[1], v1->pos[1], &i, positive);
		r.pos[2] = clip_lerp(v0->pos[2], v1->pos[2], &i, positive);
		break;
	case 1:
		r.pos[1] = positive ? r.pos[3] : -r.pos[3];
		r.pos[0] = clip_lerp(v0->pos[0], v1->pos[0], &i, positive);
		r.pos[2] = clip_lerp(v0->pos[2], v1->pos[2], &i, positive);
		break;
	case 2:
		r.pos[2] = positive ? r.pos[3] : -r.pos[3];
		r.pos[0] = clip_lerp(v0->pos[0], v1->pos[0], &i, positive);
		r.pos[1] = clip_lerp(v0->pos[1], v1->pos[1], &i, positive);
		break;
	}

	for (u32 j = 0; j < 5; j++) {
		r.attr[j] = clip_lerp(v0->attr[j], v1->attr[j], &i, positive);
	}

	return r;
}

static s32
clip_lerp(s32 y0, s32 y1, interpolator *i, int positive)
{
	if (i->denom == 0) {
		return y0;
	}

	if (positive) {
		return ((s64)y0 * (i->w1 - i->x1) +
				       (s64)y1 * (i->x0 - i->w0)) /
		       i->denom;
	} else {
		return ((s64)y1 * (i->x0 + i->w0) -
				       (s64)y0 * (i->w1 + i->x1)) /
		       i->denom;
	}
}

static void
add_polygon(geometry_engine *ge)
{
	polygon_ram *pr = ge->poly_ram;
	vertex_ram *vr = ge->vtx_ram;

	if (pr->count >= 2048) {
		LOG("polygon ram full\n");
		return;
	}

	u32 num_vtxs = 0;
	std::array<vertex *, 4> vertices{};
	std::array<vertex *, 2> last_strip_vtx_s = ge->last_strip_vtx;

	switch (ge->primitive_type) {
	case 0:
		num_vtxs = 3;
		vertices[0] = &ge->vtx_buf[(ge->vtx_count - 3) & 3];
		vertices[1] = &ge->vtx_buf[(ge->vtx_count - 2) & 3];
		vertices[2] = &ge->vtx_buf[(ge->vtx_count - 1) & 3];
		break;
	case 1:
		num_vtxs = 4;
		vertices[0] = &ge->vtx_buf[(ge->vtx_count - 4) & 3];
		vertices[1] = &ge->vtx_buf[(ge->vtx_count - 3) & 3];
		vertices[2] = &ge->vtx_buf[(ge->vtx_count - 2) & 3];
		vertices[3] = &ge->vtx_buf[(ge->vtx_count - 1) & 3];
		break;
	case 2:
		num_vtxs = 3;
		if (ge->last_strip_vtx[0]) {
			vertices[0] = ge->last_strip_vtx[0];
			vertices[0]->reused = 2;
		} else {
			vertices[0] = &ge->vtx_buf[(ge->vtx_count - 3) & 3];
			vertices[0]->reused = 0;
		}
		vertices[0]->strip_vtx = 0;

		if (ge->last_strip_vtx[1]) {
			vertices[1] = ge->last_strip_vtx[1];
			vertices[1]->reused = 1;
		} else {
			vertices[1] = &ge->vtx_buf[(ge->vtx_count - 2) & 3];
			vertices[1]->reused = 0;
		}
		vertices[1]->strip_vtx = 2;

		vertices[2] = &ge->vtx_buf[(ge->vtx_count - 1) & 3];
		vertices[2]->reused = 0;
		vertices[2]->strip_vtx = 1;

		if (ge->vtx_count % 2 == 0) {
			std::swap(vertices[1], vertices[2]);
		}
		break;
	case 3:
		num_vtxs = 4;
		if (ge->last_strip_vtx[0]) {
			vertices[0] = ge->last_strip_vtx[0];
			vertices[0]->reused = 2;
		} else {
			vertices[0] = &ge->vtx_buf[(ge->vtx_count - 4) & 3];
			vertices[0]->reused = 0;
		}
		vertices[0]->strip_vtx = 0;

		if (ge->last_strip_vtx[1]) {
			vertices[1] = ge->last_strip_vtx[1];
			vertices[1]->reused = 1;
		} else {
			vertices[1] = &ge->vtx_buf[(ge->vtx_count - 3) & 3];
			vertices[1]->reused = 0;
		}
		vertices[1]->strip_vtx = 0;

		vertices[2] = &ge->vtx_buf[(ge->vtx_count - 1) & 3];
		vertices[2]->reused = 0;
		vertices[2]->strip_vtx = 1;
		vertices[3] = &ge->vtx_buf[(ge->vtx_count - 2) & 3];
		vertices[3]->reused = 0;
		vertices[3]->strip_vtx = 2;
	}

	ge->last_strip_vtx.fill(nullptr);

	s64 face_dir = get_face_direction(
			vertices[0], vertices[1], vertices[2]);
	if ((!(ge->poly_attr & BIT(6)) && face_dir < 0) ||
			(!(ge->poly_attr & BIT(7)) && face_dir > 0)) {
		return;
	}

	std::vector<vertex> clipped_vertices = ge_clip_polygon(
			vertices, num_vtxs, ge->poly_attr & BIT(12));
	if (clipped_vertices.empty()) {
		return;
	}

	num_vtxs = clipped_vertices.size();
	if (vr->count + num_vtxs > 6144) {
		LOG("vertex ram full\n");
		return;
	}

	bool render_poly = ge->poly_attr & BIT(13);
	int max_bits = 0;
	for (u32 i = 0; i < num_vtxs; i++) {
		vertex *v = &clipped_vertices[i];
		if (v->pos[3] >= 0) {
			max_bits = std::max(max_bits,
					1 + std::bit_width((u32)v->pos[3]));
		} else {
			max_bits = std::max(max_bits,
					1 + std::bit_width(~(u32)v->pos[3]));
		}

		if (v->pos[3] == 0) {
			v->sx = 0;
			v->sy = 0;
		} else {
			s32 x = v->pos[0];
			s32 y = v->pos[1];
			s64 w = v->pos[3];
			v->sx = (x + w) * ge->viewport_w / (2 * w) +
			        ge->viewport_x[0];
			v->sy = (-y + w) * ge->viewport_h / (2 * w) +
			        ge->viewport_y[0];
		}

		if (v->sx != clipped_vertices[0].sx ||
				v->sy != clipped_vertices[0].sy) {
			render_poly = true;
		}

		if (v->pos[3] <= ge->disp_1dot_depth) {
			render_poly = true;
		}
	}

	if (!render_poly) {
		return;
	}

	polygon *p = &pr->polys[pr->count++];
	for (u32 i = 0; i < num_vtxs; i++) {
		u32 reused = clipped_vertices[i].reused;
		if (reused) {
			p->vtxs[i] = last_strip_vtx_s[reused & 1];
		} else {
			u32 idx = vr->count++;
			vr->vtxs[idx] = clipped_vertices[i];
			p->vtxs[i] = &vr->vtxs[idx];
		}
		u32 strip_vtx = clipped_vertices[i].strip_vtx;
		if (strip_vtx) {
			ge->last_strip_vtx[strip_vtx & 1] = p->vtxs[i];
		}
	}
	p->num_vtxs = num_vtxs;

	p->wshift = (max_bits - 16 + 4 - 1) & ~3;
	p->wbuffering = ge->swap_bits & BIT(1);
	for (u32 i = 0; i < p->num_vtxs; i++) {
		vertex *v = p->vtxs[i];
		if (p->wshift >= 0) {
			p->w[i] = v->pos[3] >> p->wshift;
		} else {
			p->w[i] = v->pos[3] << -p->wshift;
		}

		if (p->wbuffering) {
			p->z[i] = p->w[i];
			if (p->wshift >= 0) {
				p->z[i] <<= p->wshift;
			} else {
				p->z[i] >>= -p->wshift;
			}
		} else if (v->pos[3] != 0) {
			p->z[i] = ((s64)v->pos[2] * 0x4000 / v->pos[3] +
						  0x3FFF) *
			          0x200;
		} else {
			LOG("w value is 0\n");
			p->z[i] = ((s64)v->pos[2] * 0x4000 + 0x3FFF) * 0x200;
		}
	}

	p->attr = ge->poly_attr;
	p->tx_param = ge->teximage_param;
	p->pltt_base = ge->pltt_base;
	p->backface = face_dir < 0;

	u32 alpha = p->attr >> 16 & 0x1F;
	u32 format = p->tx_param >> 26 & 7;
	p->translucent = (1 <= alpha && alpha <= 30) ||
	                 (format == 1 || format == 6);
}

static s64
get_face_direction(vertex *v0, vertex *v1, vertex *v2)
{
	s32 ax = v1->pos[0] - v0->pos[0];
	s32 ay = v1->pos[1] - v0->pos[1];
	s32 az = v1->pos[3] - v0->pos[3];
	s32 bx = v2->pos[0] - v0->pos[0];
	s32 by = v2->pos[1] - v0->pos[1];
	s32 bz = v2->pos[3] - v0->pos[3];

	s64 cx = (s64)ay * bz - (s64)az * by;
	s64 cy = (s64)az * bx - (s64)ax * bz;
	s64 cz = (s64)ax * by - (s64)ay * bx;

	int max_bits = 0;
	max_bits = std::max(max_bits,
			1 + std::bit_width((cx >= 0 ? (u64)cx : ~(u64)cx)));
	max_bits = std::max(max_bits,
			1 + std::bit_width((cy >= 0 ? (u64)cy : ~(u64)cy)));
	max_bits = std::max(max_bits,
			1 + std::bit_width((cz >= 0 ? (u64)cz : ~(u64)cz)));
	int shift = (max_bits - 32 + 4 - 1) & ~3;
	if (shift > 0) {
		cx >>= shift;
		cy >>= shift;
		cz >>= shift;
	}

	return cx * v0->pos[0] + cy * v0->pos[1] + cz * v0->pos[3];
}

static void
texcoord_transform_3(geometry_engine *ge, s64 vx, s64 vy, s64 vz)
{
	auto& m = ge->texture_mtx;

	s64 r0 = (vx * m[0] + vy * m[4] + vz * m[8]) >> 24;
	s64 r1 = (vx * m[1] + vy * m[5] + vz * m[9]) >> 24;

	ge->vtx_texcoord[0] = ge->texcoord[0] + r0;
	ge->vtx_texcoord[1] = ge->texcoord[1] + r1;
}

} // namespace twice
