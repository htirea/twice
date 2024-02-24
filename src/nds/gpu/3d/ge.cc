#include "nds/gpu/3d/ge.h"

#include "libtwice/util/matrix.h"
#include "nds/nds.h"

namespace twice {

using ge_matrix = std::span<s32, 16>;
using ge_vector = std::span<s32, 4>;
using ge_vector3 = std::span<s32, 3>;
using const_ge_matrix = std::span<const s32, 16>;
using const_ge_vector = std::span<const s32, 4>;
using const_ge_vector3 = std::span<const s32, 3>;
using ge_params = std::span<const u32, 32>;

/* commands */
static void cmd_mtx_mode(geometry_engine *);
static void cmd_mtx_push(geometry_engine *);
static void cmd_mtx_pop(geometry_engine *);
static void cmd_mtx_store(geometry_engine *);
static void cmd_mtx_restore(geometry_engine *);
static void cmd_mtx_identity(geometry_engine *);
static void cmd_mtx_load_4x4(geometry_engine *);
static void cmd_mtx_load_4x3(geometry_engine *);
static void cmd_mtx_mult_4x4(geometry_engine *);
static void cmd_mtx_mult_4x3(geometry_engine *);
static void cmd_mtx_mult_3x3(geometry_engine *);
static void cmd_mtx_scale(geometry_engine *);
static void cmd_mtx_trans(geometry_engine *);
static void cmd_color(geometry_engine *);
static void cmd_normal(geometry_engine *);
static void cmd_texcoord(geometry_engine *);
static void cmd_vtx_16(geometry_engine *);
static void cmd_vtx_10(geometry_engine *);
static void cmd_vtx_xy(geometry_engine *);
static void cmd_vtx_xz(geometry_engine *);
static void cmd_vtx_yz(geometry_engine *);
static void cmd_vtx_diff(geometry_engine *);
static void cmd_polygon_attr(geometry_engine *);
static void cmd_teximage_param(geometry_engine *);
static void cmd_pltt_base(geometry_engine *);
static void cmd_dif_amb(geometry_engine *);
static void cmd_spe_emi(geometry_engine *);
static void cmd_light_vector(geometry_engine *);
static void cmd_light_color(geometry_engine *);
static void cmd_shininess(geometry_engine *);
static void cmd_begin_vtxs(geometry_engine *);
static void cmd_swap_buffers(geometry_engine *);
static void cmd_viewport(geometry_engine *);
static void cmd_box_test(geometry_engine *);
static void cmd_pos_test(geometry_engine *);
static void cmd_vec_test(geometry_engine *);

/* matrix */
static void mtx_mult_mtx(ge_matrix r, const_ge_matrix s, const_ge_matrix t);
static void mtx_mult_vec(ge_vector r, const_ge_matrix s, const_ge_vector t);
static void mtx_mult_vec3(
		ge_vector3 r, const_ge_matrix s, s32 t0, s32 t1, s32 t2);
static void mtx_set_identity(ge_matrix r);
static void mtx_load_4x4(ge_matrix r, ge_params t);
static void mtx_load_4x3(ge_matrix r, ge_params t);
static void mtx_mult_4x4(ge_matrix r, ge_params t);
static void mtx_mult_4x3(ge_matrix r, ge_params t);
static void mtx_mult_3x3(ge_matrix r, ge_params t);
static void mtx_scale(ge_matrix r, ge_params t);
static void mtx_trans(ge_matrix r, ge_params t);

/* clipping */
static std::vector<vertex> clip_polygon(std::span<vertex *>, u32 num_vertices,
		bool render_clipped_far);
static void clip_polygon_plane(std::list<vertex>& in, int plane, int positive,
		bool render_clipped);
static vertex create_clipped_vertex(std::list<vertex>::iterator v0,
		std::list<vertex>::iterator v1, int plane, int positive);
static s32 clip_lerp(s32 y0, s32 y1, interpolator *i, int positive);

/* polygon */
static void add_vertex(geometry_engine *ge);
static void add_polygon(geometry_engine *ge);
static s64 get_poly_face_dir(vertex *v0, vertex *v1, vertex *v2);

/* misc helpers */
static void update_proj_sp(geometry_engine *);
static void update_position_sp(geometry_engine *, s32 offset);
static void update_clip_matrix(geometry_engine *);
static void texcoord_transform_1(geometry_engine *);
static void texcoord_transform_2(geometry_engine *, s64 nx, s64 ny, s64 nz);
static void texcoord_transform_3(geometry_engine *ge, s64 vx, s64 vy, s64 vz);

void
ge_execute_command(geometry_engine *ge, u8 command)
{
	switch (command) {
	case 0x00:
		break;
	case 0x10:
		cmd_mtx_mode(ge);
		break;
	case 0x11:
		cmd_mtx_push(ge);
		break;
	case 0x12:
		cmd_mtx_pop(ge);
		break;
	case 0x13:
		cmd_mtx_store(ge);
		break;
	case 0x14:
		cmd_mtx_restore(ge);
		break;
	case 0x15:
		cmd_mtx_identity(ge);
		break;
	case 0x16:
		cmd_mtx_load_4x4(ge);
		break;
	case 0x17:
		cmd_mtx_load_4x3(ge);
		break;
	case 0x18:
		cmd_mtx_mult_4x4(ge);
		break;
	case 0x19:
		cmd_mtx_mult_4x3(ge);
		break;
	case 0x1A:
		cmd_mtx_mult_3x3(ge);
		break;
	case 0x1B:
		cmd_mtx_scale(ge);
		break;
	case 0x1C:
		cmd_mtx_trans(ge);
		break;
	case 0x20:
		cmd_color(ge);
		break;
	case 0x21:
		cmd_normal(ge);
		break;
	case 0x22:
		cmd_texcoord(ge);
		break;
	case 0x23:
		cmd_vtx_16(ge);
		break;
	case 0x24:
		cmd_vtx_10(ge);
		break;
	case 0x25:
		cmd_vtx_xy(ge);
		break;
	case 0x26:
		cmd_vtx_xz(ge);
		break;
	case 0x27:
		cmd_vtx_yz(ge);
		break;
	case 0x28:
		cmd_vtx_diff(ge);
		break;
	case 0x29:
		cmd_polygon_attr(ge);
		break;
	case 0x2A:
		cmd_teximage_param(ge);
		break;
	case 0x2B:
		cmd_pltt_base(ge);
		break;
	case 0x30:
		cmd_dif_amb(ge);
		break;
	case 0x31:
		cmd_spe_emi(ge);
		break;
	case 0x32:
		cmd_light_vector(ge);
		break;
	case 0x33:
		cmd_light_color(ge);
		break;
	case 0x34:
		cmd_shininess(ge);
		break;
	case 0x40:
		cmd_begin_vtxs(ge);
		break;
	case 0x41:
		/* END VTXS */
		break;
	case 0x50:
		cmd_swap_buffers(ge);
		break;
	case 0x60:
		cmd_viewport(ge);
		break;
	case 0x70:
		cmd_box_test(ge);
		break;
	case 0x71:
		cmd_pos_test(ge);
		break;
	case 0x72:
		cmd_vec_test(ge);
		break;
	default:
		throw twice_error("invalid ge command");
	}
}

static void
cmd_mtx_mode(geometry_engine *ge)
{
	ge->mtx_mode = ge->cmd_params[0] & 3;
}

static void
cmd_mtx_push(geometry_engine *ge)
{
	bool err = false;

	switch (ge->mtx_mode) {
	case 0:
		err = ge->proj_sp == 1;
		ge->proj_stack[0] = ge->proj_mtx;
		update_proj_sp(ge);
		break;
	case 1:
	case 2:
		err = ge->proj_sp >= 31;
		ge->position_stack[ge->position_sp & 31] = ge->position_mtx;
		ge->vector_stack[ge->position_sp & 31] = ge->vector_mtx;
		update_position_sp(ge, 1);
		break;
	case 3:
		err = ge->texture_sp == 1;
		ge->texture_stack[0] = ge->texture_mtx;
		ge->texture_sp ^= 1;
	}

	if (err) {
		ge->gpu->gxstat |= BIT(15);
	}
}

static void
cmd_mtx_pop(geometry_engine *ge)
{
	bool err = false;

	switch (ge->mtx_mode) {
	case 0:
		update_proj_sp(ge);
		err = ge->proj_sp == 1;
		ge->proj_mtx = ge->proj_stack[0];
		break;
	case 1:
	case 2:
	{
		s32 offset = SEXT<6>(ge->cmd_params[0]);
		update_position_sp(ge, -offset);
		err = ge->position_sp >= 31;
		ge->position_mtx = ge->position_stack[ge->position_sp & 31];
		ge->vector_mtx = ge->vector_stack[ge->position_sp & 31];
		break;
	}
	case 3:
		ge->texture_sp ^= 1;
		err = ge->texture_sp == 1;
		ge->texture_mtx = ge->texture_stack[0];
		break;
	}

	if (err) {
		ge->gpu->gxstat |= BIT(15);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_store(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		ge->proj_stack[0] = ge->proj_mtx;
		break;
	case 1:
	case 2:
	{
		u32 offset = ge->cmd_params[0] & 0x1F;
		if (offset == 31) {
			ge->gpu->gxstat |= BIT(15);
		}
		ge->position_stack[offset] = ge->position_mtx;
		ge->vector_stack[offset] = ge->vector_mtx;
		break;
	}
	case 3:
		ge->texture_stack[0] = ge->texture_mtx;
		break;
	}
}

static void
cmd_mtx_restore(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		ge->proj_mtx = ge->proj_stack[0];
		break;
	case 1:
	case 2:
	{
		u32 offset = ge->cmd_params[0] & 0x1F;
		if (offset == 31) {
			ge->gpu->gxstat |= BIT(15);
		}
		ge->position_mtx = ge->position_stack[offset];
		ge->vector_mtx = ge->vector_stack[offset];
		break;
	}
	case 3:
		ge->texture_mtx = ge->texture_stack[0];
		break;
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_identity(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_set_identity(ge->proj_mtx);
		ge->clip_mtx = ge->position_mtx;
		break;
	case 1:
		mtx_set_identity(ge->position_mtx);
		ge->clip_mtx = ge->proj_mtx;
		break;
	case 2:
		mtx_set_identity(ge->position_mtx);
		mtx_set_identity(ge->vector_mtx);
		ge->clip_mtx = ge->proj_mtx;
		break;
	case 3:
		mtx_set_identity(ge->texture_mtx);
	}
}

static void
cmd_mtx_load_4x4(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_load_4x4(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		mtx_load_4x4(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		mtx_load_4x4(ge->position_mtx, ge->cmd_params);
		mtx_load_4x4(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		mtx_load_4x4(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_load_4x3(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_load_4x3(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		mtx_load_4x3(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		mtx_load_4x3(ge->position_mtx, ge->cmd_params);
		mtx_load_4x3(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		mtx_load_4x3(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_mult_4x4(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_mult_4x4(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		mtx_mult_4x4(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		mtx_mult_4x4(ge->position_mtx, ge->cmd_params);
		mtx_mult_4x4(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		mtx_mult_4x4(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_mult_4x3(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_mult_4x3(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		mtx_mult_4x3(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		mtx_mult_4x3(ge->position_mtx, ge->cmd_params);
		mtx_mult_4x3(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		mtx_mult_4x3(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_mult_3x3(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_mult_3x3(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		mtx_mult_3x3(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		mtx_mult_3x3(ge->position_mtx, ge->cmd_params);
		mtx_mult_3x3(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		mtx_mult_3x3(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_scale(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_scale(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
	case 2:
		mtx_scale(ge->position_mtx, ge->cmd_params);
		break;
	case 3:
		mtx_scale(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_mtx_trans(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		mtx_trans(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		mtx_trans(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		mtx_trans(ge->position_mtx, ge->cmd_params);
		mtx_trans(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		mtx_trans(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_color(geometry_engine *ge)
{
	unpack_bgr555(ge->cmd_params[0], ge->vtx_color);
}

static void
cmd_normal(geometry_engine *ge)
{
	u32 param = ge->cmd_params[0];
	s16 nx = (s16)(param << 6) >> 6;
	s16 ny = (s16)(param >> 4) >> 6;
	s16 nz = (s16)(param >> 14) >> 6;

	if (ge->teximage_param >> 30 == 2) {
		texcoord_transform_2(ge, nx, ny, nz);
	}

	s32 normal[3];
	mtx_mult_vec3(normal, ge->vector_mtx, nx, ny, nz);

	s32 color[3]{};
	color[0] = ge->emission_color[0] << 14;
	color[1] = ge->emission_color[1] << 14;
	color[2] = ge->emission_color[2] << 14;

	for (u32 i = 0; i < 4; i++) {
		if (!(ge->poly_attr & BIT(i)))
			continue;

		s32 dif_dot = 0;
		dif_dot += -ge->light_vec[i][0] * normal[0] >> 9;
		dif_dot += -ge->light_vec[i][1] * normal[1] >> 9;
		dif_dot += -ge->light_vec[i][2] * normal[2] >> 9;
		s32 dif_level = dif_dot;

		s32 spe_dot = dif_dot + normal[2];
		if (spe_dot >= 0x400) {
			spe_dot = (0x400 - (spe_dot - 0x400)) & 0x3FF;
		}

		s32 spe_recip = ((s32)1 << 18) /
		                (-ge->light_vec[i][2] + (1 << 9));
		s32 spe_level = (spe_dot * spe_dot >> 10) * spe_recip >> 8;
		spe_level -= (1 << 9);

		spe_level = SEXT<14>(spe_level);
		if (spe_level >= 512) {
			spe_level = 511;
		}

		if (ge->shiny_table_enabled) {
			s32 idx = std::clamp<s32>(spe_level >> 2, 0, 127);
			spe_level = ge->shiny_table[idx] << 1;
		}

		for (u32 j = 0; j < 3; j++) {
			s32 l = ge->light_color[i][j];

			s32 dif = 0;
			s32 dif_color = ge->diffuse_color[j];
			if (dif_level < 0 || dif_color == 0 || l == 0) {
				dif = 0;
			} else {
				if (dif_level >= 1024) {
					dif = (512 - (dif_color * l - 512)) *
					      1024;
					dif += dif_color * l *
					       (dif_level - 1024);
				} else {
					dif = dif_color * l * dif_level;
				}
				dif = std::clamp<s32>(dif, 0, (s32)31 << 14);
			}
			color[j] += dif;

			s32 spe = 0;
			s32 spe_color = ge->specular_color[j];
			if (dif_dot <= 0 || spe_level < 0 || spe_color == 0 ||
					l == 0) {
				spe = 0;
			} else {
				spe = spe_color * l * spe_level;
			}
			color[j] += spe;

			s32 amb = ge->ambient_color[j] * l << 9;
			color[j] += amb;
		}
	}

	ge->vtx_color[0] = std::clamp<s32>(color[0] >> 14, 0, 31);
	ge->vtx_color[1] = std::clamp<s32>(color[1] >> 14, 0, 31);
	ge->vtx_color[2] = std::clamp<s32>(color[2] >> 14, 0, 31);
}

static void
cmd_texcoord(geometry_engine *ge)
{
	u32 param = ge->cmd_params[0];
	ge->texcoord[0] = (s32)(param << 16) >> 16;
	ge->texcoord[1] = (s32)(param) >> 16;

	switch (ge->teximage_param >> 30) {
	case 0:
		ge->vtx_texcoord = ge->texcoord;
		break;
	case 1:
		texcoord_transform_1(ge);
	}
}

static void
cmd_vtx_16(geometry_engine *ge)
{
	ge->vx = (s32)(ge->cmd_params[0] << 16) >> 16;
	ge->vy = (s32)(ge->cmd_params[0]) >> 16;
	ge->vz = (s32)(ge->cmd_params[1] << 16) >> 16;
	add_vertex(ge);
}

static void
cmd_vtx_10(geometry_engine *ge)
{
	ge->vx = (s32)(ge->cmd_params[0] << 22) >> 22 << 6;
	ge->vy = (s32)(ge->cmd_params[0] << 12) >> 22 << 6;
	ge->vz = (s32)(ge->cmd_params[0] << 2) >> 22 << 6;
	add_vertex(ge);
}

static void
cmd_vtx_xy(geometry_engine *ge)
{
	ge->vx = (s32)(ge->cmd_params[0] << 16) >> 16;
	ge->vy = (s32)(ge->cmd_params[0]) >> 16;
	add_vertex(ge);
}

static void
cmd_vtx_xz(geometry_engine *ge)
{
	ge->vx = (s32)(ge->cmd_params[0] << 16) >> 16;
	ge->vz = (s32)(ge->cmd_params[0]) >> 16;
	add_vertex(ge);
}

static void
cmd_vtx_yz(geometry_engine *ge)
{
	ge->vy = (s32)(ge->cmd_params[0] << 16) >> 16;
	ge->vz = (s32)(ge->cmd_params[0]) >> 16;
	add_vertex(ge);
}

static void
cmd_vtx_diff(geometry_engine *ge)
{
	ge->vx += (s16)(ge->cmd_params[0] << 6) >> 6;
	ge->vy += (s16)(ge->cmd_params[0] >> 4) >> 6;
	ge->vz += (s16)(ge->cmd_params[0] >> 14) >> 6;
	add_vertex(ge);
}

static void
cmd_polygon_attr(geometry_engine *ge)
{
	ge->poly_attr_s = ge->cmd_params[0];
}

static void
cmd_teximage_param(geometry_engine *ge)
{
	ge->teximage_param = ge->cmd_params[0];
}

static void
cmd_pltt_base(geometry_engine *ge)
{
	ge->pltt_base = ge->cmd_params[0] & 0x1FFF;
}

static void
cmd_dif_amb(geometry_engine *ge)
{
	u32 param = ge->cmd_params[0];

	unpack_bgr555(param, ge->diffuse_color);

	if (param & BIT(15)) {
		ge->vtx_color = ge->diffuse_color;
	}

	unpack_bgr555(param >> 16, ge->ambient_color);
}

static void
cmd_spe_emi(geometry_engine *ge)
{
	u32 param = ge->cmd_params[0];
	unpack_bgr555(param, ge->specular_color);
	unpack_bgr555(param >> 16, ge->emission_color);
	ge->shiny_table_enabled = param & BIT(15);
}

static void
cmd_light_vector(geometry_engine *ge)
{
	u32 param = ge->cmd_params[0];
	s16 lx = (s16)(param << 6) >> 6;
	s16 ly = (s16)(param >> 4) >> 6;
	s16 lz = (s16)(param >> 14) >> 6;
	u32 l = param >> 30;

	mtx_mult_vec3(ge->light_vec[l], ge->vector_mtx, lx, ly, lz);
	ge->half_vec[l][0] = ge->light_vec[l][0];
	ge->half_vec[l][1] = ge->light_vec[l][1];
	ge->half_vec[l][2] = (ge->light_vec[l][2] - (1 << 9));
}

static void
cmd_light_color(geometry_engine *ge)
{
	u32 l = ge->cmd_params[0] >> 30;
	unpack_bgr555(ge->cmd_params[0], ge->light_color[l]);
}

static void
cmd_shininess(geometry_engine *ge)
{
	for (u32 i = 0; i < 32; i++) {
		u32 param = ge->cmd_params[i];
		for (u32 j = 0; j < 4; j++) {
			ge->shiny_table[4 * i + j] = param >> 8 * j & 0xFF;
		}
	}
}

static void
cmd_begin_vtxs(geometry_engine *ge)
{
	ge->poly_attr = ge->poly_attr_s;
	ge->primitive_type = ge->cmd_params[0] & 3;
	ge->vtx_count = 0;
	ge->last_strip_vtx.fill(nullptr);
}

static void
cmd_swap_buffers(geometry_engine *ge)
{
	ge->gpu->halted = true;
	ge->swap_bits = ge->cmd_params[0] & 3;
}

static void
cmd_viewport(geometry_engine *ge)
{
	ge->viewport_x[0] = ge->cmd_params[0];
	ge->viewport_y[0] = ge->cmd_params[0] >> 8;
	ge->viewport_x[1] = ge->cmd_params[0] >> 16;
	ge->viewport_y[1] = ge->cmd_params[0] >> 24;
	ge->viewport_w = ge->viewport_x[1] - ge->viewport_x[0] + 1;
	ge->viewport_h = ge->viewport_y[1] - ge->viewport_y[0] + 1;
}

static void
cmd_box_test(geometry_engine *ge)
{
	s16 x = ge->cmd_params[0];
	s16 y = ge->cmd_params[0] >> 16;
	s16 z = ge->cmd_params[1];
	s16 dx = ge->cmd_params[1] >> 16;
	s16 dy = ge->cmd_params[2];
	s16 dz = ge->cmd_params[2] >> 16;

	vertex vs[8];
	std::array<s32, 4> positions[8] = {
		{ x, y, z, 1 << 12 },
		{ x + dx, y, z, 1 << 12 },
		{ x + dx, y + dy, z, 1 << 12 },
		{ x, y + dy, z, 1 << 12 },
		{ x, y, z + dz, 1 << 12 },
		{ x + dx, y, z + dz, 1 << 12 },
		{ x + dx, y + dy, z + dz, 1 << 12 },
		{ x, y + dy, z + dz, 1 << 12 },
	};

	for (u32 i = 0; i < 8; i++) {
		mtx_mult_vec(vs[i].pos, ge->clip_mtx, positions[i]);
	}

	vertex *faces[6][4] = {
		{ &vs[0], &vs[1], &vs[2], &vs[3] },
		{ &vs[1], &vs[5], &vs[6], &vs[2] },
		{ &vs[5], &vs[4], &vs[7], &vs[6] },
		{ &vs[4], &vs[0], &vs[3], &vs[7] },
		{ &vs[7], &vs[6], &vs[2], &vs[3] },
		{ &vs[4], &vs[5], &vs[1], &vs[0] },
	};

	for (u32 i = 0; i < 6; i++) {
		if (clip_polygon(faces[i], 4, true).size() > 0) {
			ge->gpu->gxstat |= BIT(1);
			return;
		}
	}

	ge->gpu->gxstat &= ~BIT(1);
}

static void
cmd_pos_test(geometry_engine *ge)
{
	s32 pos[4]{};
	pos[0] = ge->vx = (s32)(ge->cmd_params[0] << 16) >> 16;
	pos[1] = ge->vy = (s32)(ge->cmd_params[0]) >> 16;
	pos[2] = ge->vz = (s32)(ge->cmd_params[1] << 16) >> 16;
	pos[3] = 1 << 12;

	mtx_mult_vec(ge->pos_test_result, ge->clip_mtx, pos);
}

static void
cmd_vec_test(geometry_engine *ge)
{
	u32 param = ge->cmd_params[0];
	s32 x = (s16)(param << 6) >> 6;
	s32 y = (s16)(param >> 4) >> 6;
	s32 z = (s16)(param >> 14) >> 6;

	s32 result[3]{};
	mtx_mult_vec3(result, ge->vector_mtx, x << 3, y << 3, z << 3);

	ge->vec_test_result[0] = (s16)(result[0] << 3) >> 3;
	ge->vec_test_result[1] = (s16)(result[1] << 3) >> 3;
	ge->vec_test_result[2] = (s16)(result[2] << 3) >> 3;
}

static void
mtx_mult_mtx(ge_matrix r, const_ge_matrix s, const_ge_matrix t)
{
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 4; k++) {
				sum += (s64)s[k * 4 + j] * t[i * 4 + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}
}

static void
mtx_mult_vec(ge_vector r, const_ge_matrix s, const_ge_vector t)
{
	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 4; k++) {
			sum += (s64)s[k * 4 + j] * t[k];
		}
		r[j] = sum >> 12;
	}
}

static void
mtx_mult_vec3(ge_vector3 r, const_ge_matrix s, s32 t0, s32 t1, s32 t2)
{
	for (u32 j = 0; j < 3; j++) {
		s64 sum = (s64)s[j] * t0 + (s64)s[4 + j] * t1 +
		          (s64)s[8 + j] * t2;
		r[j] = sum >> 12;
	}
}

static void
mtx_set_identity(ge_matrix r)
{
	mtx_set_identity<s32, 4>(r, 1 << 12);
}

static void
mtx_load_4x4(ge_matrix r, ge_params t)
{
	for (u32 i = 0; i < 16; i++) {
		r[i] = t[i];
	}
}

static void
mtx_load_4x3(ge_matrix r, ge_params ts)
{
	auto t = ts.data();

	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 3; j++) {
			r[i * 4 + j] = *t++;
		}
	}

	r[3] = 0;
	r[7] = 0;
	r[11] = 0;
	r[15] = 1 << 12;
}

static void
mtx_mult_4x4(ge_matrix r, ge_params t)
{
	std::array<s32, r.size()> s;
	std::ranges::copy(r, s.begin());

	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 4; k++) {
				sum += (s64)s[k * 4 + j] * (s32)t[4 * i + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}
}

static void
mtx_mult_4x3(ge_matrix r, ge_params t)
{
	std::array<s32, r.size()> s;
	std::ranges::copy(r, s.begin());

	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 3; k++) {
				sum += (s64)s[k * 4 + j] * (s32)t[3 * i + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}

	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 3; k++) {
			sum += (s64)s[k * 4 + j] * (s32)t[9 + k];
		}
		sum += (s64)s[12 + j] << 12;
		r[12 + j] = sum >> 12;
	}
}

static void
mtx_mult_3x3(ge_matrix r, ge_params t)
{
	std::array<s32, r.size()> s;
	std::ranges::copy(r, s.begin());

	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 3; k++) {
				sum += (s64)s[k * 4 + j] * (s32)t[3 * i + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}

	for (u32 j = 0; j < 4; j++) {
		r[12 + j] = s[12 + j];
	}
}

static void
mtx_scale(ge_matrix r, ge_params t)
{
	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			r[i * 4 + j] = (s64)r[i * 4 + j] * (s32)t[i] >> 12;
		}
	}
}

static void
mtx_trans(ge_matrix r, ge_params t)
{
	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 3; k++) {
			sum += (s64)r[k * 4 + j] * (s32)t[k];
		}
		sum += (s64)r[12 + j] << 12;
		r[12 + j] = sum >> 12;
	}
}

static std::vector<vertex>
clip_polygon(std::span<vertex *> in, u32 num_vertices, bool render_clipped_far)
{
	std::list<vertex> out;
	for (u32 i = 0; i < num_vertices; i++) {
		out.push_back(*in[i]);
	}

	clip_polygon_plane(out, 2, 1, render_clipped_far);
	clip_polygon_plane(out, 2, 0, true);
	clip_polygon_plane(out, 1, 1, true);
	clip_polygon_plane(out, 1, 0, true);
	clip_polygon_plane(out, 0, 1, true);
	clip_polygon_plane(out, 0, 0, true);

	return { out.begin(), out.end() };
}

static void
clip_polygon_plane(std::list<vertex>& in, int plane, int positive,
		bool render_clipped)
{
	if (in.empty())
		return;

	in.push_back(in.front());
	auto end = --in.end();

	for (auto curr = in.begin(); curr != end;) {
		auto next = std::next(curr);
		bool curr_inside =
				positive ? curr->pos[plane] <= curr->pos[3]
					 : curr->pos[plane] >= -curr->pos[3];
		bool next_inside =
				positive ? next->pos[plane] <= next->pos[3]
					 : next->pos[plane] >= -next->pos[3];

		if (!(curr_inside && next_inside) && !render_clipped) {
			in.clear();
			return;
		}

		if (curr_inside && next_inside) {
			curr = next;
		} else if (curr_inside) {
			in.insert(next, create_clipped_vertex(curr, next,
							plane, positive));
			curr = next;
		} else if (next_inside) {
			in.insert(next, create_clipped_vertex(curr, next,
							plane, positive));
			in.erase(curr);
			curr = next;
		} else {
			curr = in.erase(curr);
		}
	}

	in.erase(end);
}

static vertex
create_clipped_vertex(std::list<vertex>::iterator v0,
		std::list<vertex>::iterator v1, int plane, int positive)
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
add_vertex(geometry_engine *ge)
{
	vertex_ram *vr = ge->vtx_ram;

	if (vr->count >= 6144) {
		LOG("vertex ram full\n");
		return;
	}

	vertex *v = &ge->vtx_buf[ge->vtx_count & 3];
	mtx_mult_vec(v->pos, ge->clip_mtx,
			std::array<s32, 4>{ ge->vx, ge->vy, ge->vz, 1 << 12 });

	v->attr[0] = ((s32)ge->vtx_color[0] << 1) + (ge->vtx_color[0] != 0);
	v->attr[1] = ((s32)ge->vtx_color[1] << 1) + (ge->vtx_color[1] != 0);
	v->attr[2] = ((s32)ge->vtx_color[2] << 1) + (ge->vtx_color[2] != 0);
	v->attr[0] <<= 3;
	v->attr[1] <<= 3;
	v->attr[2] <<= 3;

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

	s64 face_dir = get_poly_face_dir(
			vertices[0], vertices[1], vertices[2]);
	if ((!(ge->poly_attr & BIT(6)) && face_dir < 0) ||
			(!(ge->poly_attr & BIT(7)) && face_dir > 0)) {
		return;
	}

	std::vector<vertex> clipped_vertices = clip_polygon(
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
			p->z[i] = ((s64)v->pos[2] * 0x800000 / v->pos[3]) +
			          0x7FFFFF;
		} else {
			LOG("w value is 0\n");
			p->z[i] = 0xFFFFFF;
		}

		p->z[i] = std::clamp(p->z[i], 0, 0xFFFFFF);
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
get_poly_face_dir(vertex *v0, vertex *v1, vertex *v2)
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
update_proj_sp(geometry_engine *ge)
{
	ge->proj_sp ^= 1;
	ge->gpu->gxstat = (ge->gpu->gxstat & ~BIT(13)) | (ge->proj_sp << 13);
}

static void
update_position_sp(geometry_engine *ge, s32 offset)
{
	ge->position_sp = (ge->position_sp + offset) & 0x3F;
	ge->gpu->gxstat = (ge->gpu->gxstat & ~0x1F00) |
	                  (ge->position_sp & 0x1F) << 8;
}

static void
update_clip_matrix(geometry_engine *ge)
{
	mtx_mult_mtx(ge->clip_mtx, ge->proj_mtx, ge->position_mtx);
}

static void
texcoord_transform_1(geometry_engine *ge)
{
	auto& m = ge->texture_mtx;

	s64 s = ge->texcoord[0];
	s64 t = ge->texcoord[1];
	s64 r0 = (s * m[0] + t * m[4] + m[8] + m[12]) >> 12;
	s64 r1 = (s * m[1] + t * m[5] + m[9] + m[13]) >> 12;

	ge->vtx_texcoord[0] = r0;
	ge->vtx_texcoord[1] = r1;
}

static void
texcoord_transform_2(geometry_engine *ge, s64 nx, s64 ny, s64 nz)
{
	auto& m = ge->texture_mtx;

	s64 r0 = (nx * m[0] + ny * m[4] + nz * m[8]) >> 21;
	s64 r1 = (nx * m[1] + ny * m[5] + nz * m[9]) >> 21;

	ge->vtx_texcoord[0] = ge->texcoord[0] + r0;
	ge->vtx_texcoord[1] = ge->texcoord[1] + r1;
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
