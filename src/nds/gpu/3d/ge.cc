#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

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

void
ge_execute_command(gpu_3d_engine *gpu, u8 command)
{
	switch (command) {
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
	case 0x50:
		gpu->halted = true;
		break;
	default:
		LOG("unhandled ge command %02X\n", command);
	}
}

} // namespace twice
