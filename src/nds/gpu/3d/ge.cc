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
	default:
		LOG("unhandled ge command %02X\n", command);
	}
}

} // namespace twice
