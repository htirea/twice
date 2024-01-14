#include "nds/gpu/3d/ge.h"

#include "libtwice/util/matrix.h"
#include "nds/gpu/3d/ge_matrix.h"
#include "nds/gpu/3d/ge_poly.h"
#include "nds/nds.h"

namespace twice {

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

static void update_proj_sp(geometry_engine *);
static void update_position_sp(geometry_engine *, s32 offset);
static void update_clip_matrix(geometry_engine *);
static void texcoord_transform_1(geometry_engine *);
static void texcoord_transform_2(geometry_engine *, s64 nx, s64 ny, s64 nz);

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
	bool err;

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
	bool err;

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
		ge_mtx_set_identity(ge->proj_mtx);
		ge->clip_mtx = ge->position_mtx;
		break;
	case 1:
		ge_mtx_set_identity(ge->position_mtx);
		ge->clip_mtx = ge->proj_mtx;
		break;
	case 2:
		ge_mtx_set_identity(ge->position_mtx);
		ge_mtx_set_identity(ge->vector_mtx);
		ge->clip_mtx = ge->proj_mtx;
		break;
	case 3:
		ge_mtx_set_identity(ge->texture_mtx);
	}
}

static void
cmd_mtx_load_4x4(geometry_engine *ge)
{
	switch (ge->mtx_mode) {
	case 0:
		ge_mtx_load_4x4(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		ge_mtx_load_4x4(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		ge_mtx_load_4x4(ge->position_mtx, ge->cmd_params);
		ge_mtx_load_4x4(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		ge_mtx_load_4x4(ge->texture_mtx, ge->cmd_params);
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
		ge_mtx_load_4x3(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		ge_mtx_load_4x3(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		ge_mtx_load_4x3(ge->position_mtx, ge->cmd_params);
		ge_mtx_load_4x3(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		ge_mtx_load_4x3(ge->texture_mtx, ge->cmd_params);
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
		ge_mtx_mult_4x4(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		ge_mtx_mult_4x4(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		ge_mtx_mult_4x4(ge->position_mtx, ge->cmd_params);
		ge_mtx_mult_4x4(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		ge_mtx_mult_4x4(ge->texture_mtx, ge->cmd_params);
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
		ge_mtx_mult_4x3(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		ge_mtx_mult_4x3(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		ge_mtx_mult_4x3(ge->position_mtx, ge->cmd_params);
		ge_mtx_mult_4x3(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		ge_mtx_mult_4x3(ge->texture_mtx, ge->cmd_params);
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
		ge_mtx_mult_3x3(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		ge_mtx_mult_3x3(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		ge_mtx_mult_3x3(ge->position_mtx, ge->cmd_params);
		ge_mtx_mult_3x3(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		ge_mtx_mult_3x3(ge->texture_mtx, ge->cmd_params);
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
		ge_mtx_scale(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
	case 2:
		ge_mtx_scale(ge->position_mtx, ge->cmd_params);
		break;
	case 3:
		ge_mtx_scale(ge->texture_mtx, ge->cmd_params);
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
		ge_mtx_trans(ge->proj_mtx, ge->cmd_params);
		break;
	case 1:
		ge_mtx_trans(ge->position_mtx, ge->cmd_params);
		break;
	case 2:
		ge_mtx_trans(ge->position_mtx, ge->cmd_params);
		ge_mtx_trans(ge->vector_mtx, ge->cmd_params);
		break;
	case 3:
		ge_mtx_trans(ge->texture_mtx, ge->cmd_params);
	}

	if (ge->mtx_mode < 3) {
		update_clip_matrix(ge);
	}
}

static void
cmd_color(geometry_engine *ge)
{
	unpack_bgr555_3d(ge->cmd_params[0], ge->vtx_color);
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
	ge_mtx_mult_vec3(normal, ge->vector_mtx, nx, ny, nz);

	ge->vtx_color = ge->emission_color;

	for (u32 i = 0; i < 4; i++) {
		if (!(ge->poly_attr & BIT(i)))
			continue;

		s64 diffuse_level = -vector_dot<s64, s32, 3>(
				ge->light_vec[i], normal);
		diffuse_level = std::clamp<s64>(
				diffuse_level, 0, (s64)1 << 18);
		diffuse_level >>= 9;

		s64 shiny_level = -vector_dot<s64, s32, 3>(
				ge->half_vec[i], normal);
		shiny_level = std::clamp<s64>(shiny_level, 0, (s64)1 << 18);
		shiny_level = (shiny_level >> 9) * (shiny_level >> 9) >> 9;

		if (ge->shiny_table_enabled) {
			s64 idx = std::clamp<s64>(shiny_level >> 2, 0, 127);
			shiny_level = ge->shiny_table[idx] << 1;
		}

		for (u32 j = 0; j < 3; j++) {
			s64 light_color = ge->light_color[i][j];
			s64 specular = ge->specular_color[j] * light_color *
			               shiny_level;
			s64 diffuse = ge->diffuse_color[j] * light_color *
			              diffuse_level;
			s64 ambient = ge->ambient_color[j] * light_color;
			s64 color = ge->vtx_color[j] + (specular >> 15) +
			            (diffuse >> 15) + (ambient >> 6);
			ge->vtx_color[j] = std::clamp<s64>(color, 0, 63);
		}
	}
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
	ge_add_vertex(ge);
}

static void
cmd_vtx_10(geometry_engine *ge)
{
	ge->vx = (s32)(ge->cmd_params[0] << 22) >> 22 << 6;
	ge->vy = (s32)(ge->cmd_params[0] << 12) >> 22 << 6;
	ge->vz = (s32)(ge->cmd_params[0] << 2) >> 22 << 6;
	ge_add_vertex(ge);
}

static void
cmd_vtx_xy(geometry_engine *ge)
{
	ge->vx = (s32)(ge->cmd_params[0] << 16) >> 16;
	ge->vy = (s32)(ge->cmd_params[0]) >> 16;
	ge_add_vertex(ge);
}

static void
cmd_vtx_xz(geometry_engine *ge)
{
	ge->vx = (s32)(ge->cmd_params[0] << 16) >> 16;
	ge->vz = (s32)(ge->cmd_params[0]) >> 16;
	ge_add_vertex(ge);
}

static void
cmd_vtx_yz(geometry_engine *ge)
{
	ge->vy = (s32)(ge->cmd_params[0] << 16) >> 16;
	ge->vz = (s32)(ge->cmd_params[0]) >> 16;
	ge_add_vertex(ge);
}

static void
cmd_vtx_diff(geometry_engine *ge)
{
	ge->vx += (s16)(ge->cmd_params[0] << 6) >> 6;
	ge->vy += (s16)(ge->cmd_params[0] >> 4) >> 6;
	ge->vz += (s16)(ge->cmd_params[0] >> 14) >> 6;
	ge_add_vertex(ge);
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

	unpack_bgr555_3d(param, ge->diffuse_color);

	if (param & BIT(15)) {
		ge->vtx_color = ge->diffuse_color;
	}

	unpack_bgr555_3d(param >> 16, ge->ambient_color);
}

static void
cmd_spe_emi(geometry_engine *ge)
{
	u32 param = ge->cmd_params[0];
	unpack_bgr555_3d(param, ge->specular_color);
	unpack_bgr555_3d(param >> 16, ge->emission_color);
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

	ge_mtx_mult_vec3(ge->light_vec[l], ge->vector_mtx, lx, ly, lz);
	ge->half_vec[l][0] = ge->light_vec[l][0] >> 1;
	ge->half_vec[l][1] = ge->light_vec[l][1] >> 1;
	ge->half_vec[l][2] = (ge->light_vec[l][2] - (1 << 9)) >> 1;
}

static void
cmd_light_color(geometry_engine *ge)
{
	u32 l = ge->cmd_params[0] >> 30;
	unpack_bgr555_3d(ge->cmd_params[0], ge->light_color[l]);
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
		ge_mtx_mult_vec(vs[i].pos, ge->clip_mtx, positions[i]);
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
		if (ge_clip_polygon(faces[i], 4, true).size() > 0) {
			ge->gpu->gxstat |= BIT(1);
			return;
		}
	}

	ge->gpu->gxstat &= ~BIT(1);
}

static void
cmd_pos_test(geometry_engine *)
{
	throw twice_error("pos test");
}

static void
cmd_vec_test(geometry_engine *)
{
	throw twice_error("vec test");
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
	ge_mtx_mult_mtx(ge->clip_mtx, ge->proj_mtx, ge->position_mtx);
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

} // namespace twice
