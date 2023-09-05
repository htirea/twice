#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

using vertex = gpu_3d_engine::vertex;
using polygon = gpu_3d_engine::polygon;

static void
update_clip_matrix(gpu_3d_engine *gpu)
{
	mtx_mult_mtx(&gpu->clip_mtx, &gpu->projection_mtx, &gpu->position_mtx);
}

static void
cmd_mtx_mode(gpu_3d_engine *gpu)
{
	gpu->mtx_mode = gpu->cmd_params[0] & 3;
}

static void
update_projection_sp(gpu_3d_engine *gpu)
{
	gpu->projection_sp ^= 1;
	gpu->gxstat = (gpu->gxstat & ~BIT(13)) | (gpu->projection_sp << 13);
}

static void
update_position_sp(gpu_3d_engine *gpu, s32 offset)
{
	gpu->position_sp = (gpu->position_sp + offset) & 0x3F;
	gpu->gxstat = (gpu->gxstat & ~0x1F00) | (gpu->position_sp & 0x1F) << 8;
}

static void
cmd_mtx_push(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		if (gpu->projection_sp == 1) {
			gpu->gxstat |= BIT(15);
		}
		gpu->projection_stack[0] = gpu->projection_mtx;
		update_projection_sp(gpu);
		break;
	case 1:
	case 2:
		if (gpu->position_sp >= 31) {
			gpu->gxstat |= BIT(15);
		}
		gpu->position_stack[gpu->position_sp & 31] = gpu->position_mtx;
		gpu->vector_stack[gpu->position_sp & 31] = gpu->vector_mtx;
		update_position_sp(gpu, 1);
		break;
	case 3:
		if (gpu->texture_sp == 1) {
			gpu->gxstat |= BIT(15);
		}
		gpu->texture_stack[0] = gpu->texture_mtx;
		gpu->texture_sp ^= 1;
	}
}

static void
cmd_mtx_pop(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		update_projection_sp(gpu);
		if (gpu->projection_sp == 1) {
			gpu->gxstat |= BIT(15);
		}
		gpu->projection_mtx = gpu->projection_stack[0];
		break;
	case 1:
	case 2:
	{
		s32 offset = SEXT<6>(gpu->cmd_params[0]);
		update_position_sp(gpu, -offset);
		if (gpu->position_sp >= 31) {
			gpu->gxstat |= BIT(15);
		}
		gpu->position_mtx = gpu->position_stack[gpu->position_sp & 31];
		gpu->vector_mtx = gpu->vector_stack[gpu->position_sp & 31];
		break;
	}
	case 3:
		gpu->texture_sp ^= 1;
		if (gpu->texture_sp == 1) {
			gpu->gxstat |= BIT(15);
		}
		gpu->texture_mtx = gpu->texture_stack[0];
		break;
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_store(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		gpu->projection_stack[0] = gpu->projection_mtx;
		break;
	case 1:
	case 2:
	{
		u32 offset = gpu->cmd_params[0] & 0x1F;
		if (offset == 31) {
			gpu->gxstat |= BIT(15);
		}
		gpu->position_stack[offset] = gpu->position_mtx;
		gpu->vector_stack[offset] = gpu->vector_mtx;
		break;
	}
	case 3:
		gpu->texture_stack[0] = gpu->texture_mtx;
		break;
	}
}

static void
cmd_mtx_restore(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		gpu->projection_mtx = gpu->projection_stack[0];
		break;
	case 1:
	case 2:
	{
		u32 offset = gpu->cmd_params[0] & 0x1F;
		if (offset == 31) {
			gpu->gxstat |= BIT(15);
		}
		gpu->position_mtx = gpu->position_stack[offset];
		gpu->vector_mtx = gpu->vector_stack[offset];
		break;
	}
	case 3:
		gpu->texture_mtx = gpu->texture_stack[0];
		break;
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_identity(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_set_identity(&gpu->projection_mtx);
		gpu->clip_mtx = gpu->position_mtx;
		break;
	case 1:
		mtx_set_identity(&gpu->position_mtx);
		gpu->clip_mtx = gpu->projection_mtx;
		break;
	case 2:
		mtx_set_identity(&gpu->position_mtx);
		mtx_set_identity(&gpu->vector_mtx);
		gpu->clip_mtx = gpu->projection_mtx;
		break;
	case 3:
		mtx_set_identity(&gpu->texture_mtx);
	}
}

static void
cmd_mtx_load_4x4(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_load_4x4(&gpu->projection_mtx, gpu->cmd_params);
		break;
	case 1:
		mtx_load_4x4(&gpu->position_mtx, gpu->cmd_params);
		break;
	case 2:
		mtx_load_4x4(&gpu->position_mtx, gpu->cmd_params);
		mtx_load_4x4(&gpu->vector_mtx, gpu->cmd_params);
		break;
	case 3:
		mtx_load_4x4(&gpu->texture_mtx, gpu->cmd_params);
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_load_4x3(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_load_4x3(&gpu->projection_mtx, gpu->cmd_params);
		break;
	case 1:
		mtx_load_4x3(&gpu->position_mtx, gpu->cmd_params);
		break;
	case 2:
		mtx_load_4x3(&gpu->position_mtx, gpu->cmd_params);
		mtx_load_4x3(&gpu->vector_mtx, gpu->cmd_params);
		break;
	case 3:
		mtx_load_4x3(&gpu->texture_mtx, gpu->cmd_params);
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_mult_4x4(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_mult_4x4(&gpu->projection_mtx, gpu->cmd_params);
		break;
	case 1:
		mtx_mult_4x4(&gpu->position_mtx, gpu->cmd_params);
		break;
	case 2:
		mtx_mult_4x4(&gpu->position_mtx, gpu->cmd_params);
		mtx_mult_4x4(&gpu->vector_mtx, gpu->cmd_params);
		break;
	case 3:
		mtx_mult_4x4(&gpu->texture_mtx, gpu->cmd_params);
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_mult_4x3(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_mult_4x3(&gpu->projection_mtx, gpu->cmd_params);
		break;
	case 1:
		mtx_mult_4x3(&gpu->position_mtx, gpu->cmd_params);
		break;
	case 2:
		mtx_mult_4x3(&gpu->position_mtx, gpu->cmd_params);
		mtx_mult_4x3(&gpu->vector_mtx, gpu->cmd_params);
		break;
	case 3:
		mtx_mult_4x3(&gpu->texture_mtx, gpu->cmd_params);
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_mult_3x3(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_mult_3x3(&gpu->projection_mtx, gpu->cmd_params);
		break;
	case 1:
		mtx_mult_3x3(&gpu->position_mtx, gpu->cmd_params);
		break;
	case 2:
		mtx_mult_3x3(&gpu->position_mtx, gpu->cmd_params);
		mtx_mult_3x3(&gpu->vector_mtx, gpu->cmd_params);
		break;
	case 3:
		mtx_mult_3x3(&gpu->texture_mtx, gpu->cmd_params);
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_scale(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_scale(&gpu->projection_mtx, gpu->cmd_params);
		break;
	case 1:
	case 2:
		mtx_scale(&gpu->position_mtx, gpu->cmd_params);
		break;
	case 3:
		mtx_scale(&gpu->texture_mtx, gpu->cmd_params);
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_mtx_trans(gpu_3d_engine *gpu)
{
	switch (gpu->mtx_mode) {
	case 0:
		mtx_trans(&gpu->projection_mtx, gpu->cmd_params);
		break;
	case 1:
		mtx_trans(&gpu->position_mtx, gpu->cmd_params);
		break;
	case 2:
		mtx_trans(&gpu->position_mtx, gpu->cmd_params);
		mtx_trans(&gpu->vector_mtx, gpu->cmd_params);
		break;
	case 3:
		mtx_trans(&gpu->texture_mtx, gpu->cmd_params);
	}

	if (gpu->mtx_mode < 3) {
		update_clip_matrix(gpu);
	}
}

static void
cmd_color(gpu_3d_engine *gpu)
{
	gpu->ge.vertex_color = bgr555_to_color6_3d(gpu->cmd_params[0]);
}

static bool
is_backface(vertex **vertices)
{
	vertex *v0 = vertices[0];
	vertex *v1 = vertices[1];
	vertex *v2 = vertices[2];

	s32 ax = v1->x - v0->x;
	s32 ay = v1->y - v0->y;
	s32 az = v1->w - v0->w;
	s32 bx = v2->x - v0->x;
	s32 by = v2->y - v0->y;
	s32 bz = v2->w - v0->w;

	s64 cx = (s64)ay * bz - (s64)az * by;
	s64 cy = (s64)az * bx - (s64)ax * bz;
	s64 cz = (s64)ax * by - (s64)ay * bx;

	s64 dot = cx * v0->x + cy * v0->y + cz * v0->w;
	return dot <= 0;
}

static void
add_polygon(gpu_3d_engine *gpu)
{
	auto& ge = gpu->ge;
	auto& pr = gpu->poly_ram[gpu->ge_buf];
	auto& vr = gpu->vtx_ram[gpu->ge_buf];

	if (pr.count >= 6144) {
		LOGV("polygon ram full\n");
		return;
	}

	u32 num_vertices = 0;
	vertex *vertices[4]{};
	vertex *last_vtx = &vr.vertices[vr.count - 1];

	switch (ge.primitive_type) {
	case 0:
		num_vertices = 3;
		for (u32 i = 3; i--;)
			vertices[i] = last_vtx--;
		break;
	case 1:
		num_vertices = 4;
		for (u32 i = 4; i--;)
			vertices[i] = last_vtx--;
		break;
	case 2:
		num_vertices = 3;
		if (ge.vtx_count % 2 == 1) {
			for (u32 i = 3; i--;)
				vertices[i] = last_vtx--;
		} else {
			vertices[0] = last_vtx - 2;
			vertices[1] = last_vtx;
			vertices[2] = last_vtx - 1;
		}
		break;
	case 3:
		num_vertices = 4;
		vertices[0] = last_vtx - 3;
		vertices[1] = last_vtx - 2;
		vertices[2] = last_vtx;
		vertices[3] = last_vtx - 1;
	}

	bool backface = is_backface(vertices);
	/* TODO: face culling */
	/* TODO: clipping */

	polygon *poly = &pr.polygons[pr.count++];
	for (u32 i = 0; i < num_vertices; i++) {
		poly->vertices[i] = vertices[i];
	}
	poly->num_vertices = num_vertices;

	for (u32 i = 0; i < poly->num_vertices; i++) {
		vertex *v = poly->vertices[i];
		if (v->w == 0) {
			v->sx = 0;
			v->sy = 0;
		} else {
			v->sx = (v->x + v->w) * gpu->viewport_w / (2 * v->w) +
			        gpu->viewport_x[0];
			v->sy = (v->y + v->w) * gpu->viewport_h / (2 * v->w) +
			        gpu->viewport_y[0];
			v->sy = 192 - v->sy;
		}
	}

	poly->attr = ge.polygon_attr;
	poly->backface = backface;
}

static void
add_vertex(gpu_3d_engine *gpu)
{
	auto& ge = gpu->ge;
	auto& vr = gpu->vtx_ram[gpu->ge_buf];

	if (vr.count >= 2048) {
		LOGV("vertex ram full\n");
		return;
	}

	ge_vector vtx_point = { ge.vx, ge.vy, ge.vz, 1 << 12 };
	ge_vector result;
	mtx_mult_vec(&result, &gpu->clip_mtx, &vtx_point);

	vr.vertices[vr.count++] = { result.v[0], result.v[1], result.v[2],
		result.v[3], ge.vertex_color };
	ge.vtx_count++;

	switch (ge.primitive_type) {
	case 0:
		if (ge.vtx_count % 3 == 0) {
			add_polygon(gpu);
		}
		break;
	case 1:
		if (ge.vtx_count % 4 == 0) {
			add_polygon(gpu);
		}
		break;
	case 2:
		if (ge.vtx_count >= 3) {
			add_polygon(gpu);
		}
		break;
	case 4:
		if (ge.vtx_count >= 4 && ge.vtx_count % 2 == 0) {
			add_polygon(gpu);
		}
	}
}

static void
cmd_vtx_16(gpu_3d_engine *gpu)
{
	gpu->ge.vx = (s32)(gpu->cmd_params[0] << 16) >> 16;
	gpu->ge.vy = (s32)(gpu->cmd_params[0]) >> 16;
	gpu->ge.vz = (s32)(gpu->cmd_params[1] << 16) >> 16;
	add_vertex(gpu);
}

static void
cmd_vtx_10(gpu_3d_engine *gpu)
{
	gpu->ge.vx = (s32)(gpu->cmd_params[0] << 22) >> 16;
	gpu->ge.vy = (s32)(gpu->cmd_params[0] << 12) >> 16;
	gpu->ge.vz = (s32)(gpu->cmd_params[0] << 2) >> 16;
	add_vertex(gpu);
}

static void
cmd_vtx_xy(gpu_3d_engine *gpu)
{
	gpu->ge.vx = (s32)(gpu->cmd_params[0] << 16) >> 16;
	gpu->ge.vy = (s32)(gpu->cmd_params[0]) >> 16;
	add_vertex(gpu);
}

static void
cmd_vtx_xz(gpu_3d_engine *gpu)
{
	gpu->ge.vx = (s32)(gpu->cmd_params[0] << 16) >> 16;
	gpu->ge.vz = (s32)(gpu->cmd_params[0]) >> 16;
	add_vertex(gpu);
}

static void
cmd_vtx_yz(gpu_3d_engine *gpu)
{
	gpu->ge.vy = (s32)(gpu->cmd_params[0] << 16) >> 16;
	gpu->ge.vz = (s32)(gpu->cmd_params[0]) >> 16;
	add_vertex(gpu);
}

static void
cmd_vtx_diff(gpu_3d_engine *gpu)
{
	gpu->ge.vx += (s32)(gpu->cmd_params[0] << 22) >> 19;
	gpu->ge.vy += (s32)(gpu->cmd_params[0] << 12) >> 19;
	gpu->ge.vz += (s32)(gpu->cmd_params[0] << 2) >> 19;
	add_vertex(gpu);
}

static void
cmd_polygon_attr(gpu_3d_engine *gpu)
{
	gpu->ge.polygon_attr_shadow = gpu->cmd_params[0];
}

static void
cmd_teximage_param(gpu_3d_engine *gpu)
{
	gpu->ge.teximage_param = gpu->cmd_params[0];
}

static void
cmd_begin_vtxs(gpu_3d_engine *gpu)
{
	gpu->ge.polygon_attr = gpu->ge.polygon_attr_shadow;
	gpu->ge.primitive_type = gpu->cmd_params[0] & 3;
	gpu->ge.vtx_count = 0;
}

static void
cmd_swap_buffers(gpu_3d_engine *gpu)
{
	gpu->halted = true;
	gpu->ge.swap_bits_s = gpu->cmd_params[0] & 3;
}

static void
cmd_viewport(gpu_3d_engine *gpu)
{
	gpu->viewport_x[0] = gpu->cmd_params[0];
	gpu->viewport_y[0] = gpu->cmd_params[0] >> 8;
	gpu->viewport_x[1] = gpu->cmd_params[0] >> 16;
	gpu->viewport_y[1] = gpu->cmd_params[0] >> 24;
	gpu->viewport_w = gpu->viewport_x[1] - gpu->viewport_x[0] + 1;
	gpu->viewport_h = gpu->viewport_y[1] - gpu->viewport_y[0] + 1;
}

void
ge_execute_command(gpu_3d_engine *gpu, u8 command)
{
	switch (command) {
	case 0x00:
		break;
	case 0x10:
		cmd_mtx_mode(gpu);
		break;
	case 0x11:
		cmd_mtx_push(gpu);
		break;
	case 0x12:
		cmd_mtx_pop(gpu);
		break;
	case 0x13:
		cmd_mtx_store(gpu);
		break;
	case 0x14:
		cmd_mtx_restore(gpu);
		break;
	case 0x15:
		cmd_mtx_identity(gpu);
		break;
	case 0x16:
		cmd_mtx_load_4x4(gpu);
		break;
	case 0x17:
		cmd_mtx_load_4x3(gpu);
		break;
	case 0x18:
		cmd_mtx_mult_4x4(gpu);
		break;
	case 0x19:
		cmd_mtx_mult_4x3(gpu);
		break;
	case 0x1A:
		cmd_mtx_mult_3x3(gpu);
		break;
	case 0x1B:
		cmd_mtx_scale(gpu);
		break;
	case 0x1C:
		cmd_mtx_trans(gpu);
		break;
	case 0x20:
		cmd_color(gpu);
		break;
	case 0x23:
		cmd_vtx_16(gpu);
		break;
	case 0x24:
		cmd_vtx_10(gpu);
		break;
	case 0x25:
		cmd_vtx_xy(gpu);
		break;
	case 0x26:
		cmd_vtx_xz(gpu);
		break;
	case 0x27:
		cmd_vtx_yz(gpu);
		break;
	case 0x28:
		cmd_vtx_diff(gpu);
		break;
	case 0x29:
		cmd_polygon_attr(gpu);
		break;
	case 0x2A:
		cmd_teximage_param(gpu);
		break;
	case 0x41:
		break;
	case 0x40:
		cmd_begin_vtxs(gpu);
		break;
	case 0x50:
		cmd_swap_buffers(gpu);
		break;
	case 0x60:
		cmd_viewport(gpu);
		break;
	default:
		LOG("unhandled ge command %02X\n", command);
	}
}

} // namespace twice
