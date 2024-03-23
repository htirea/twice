#include "nds/nds.h"

#include "nds/gpu/2d/render.h"

#include "common/logger.h"

namespace twice {

static void check_window_y_bounds(gpu_2d_engine *gpu, u32 y);

void
gpu2d_init(nds_ctx *nds)
{
	gpu_2d_engine *gpu_a = &nds->gpu2d[0];
	gpu_2d_engine *gpu_b = &nds->gpu2d[1];

	gpu_a->nds = nds;
	gpu_a->engineid = 0;
	gpu_a->palette_offset = 0x000;

	gpu_b->nds = nds;
	gpu_b->engineid = 1;
	gpu_b->palette_offset = 0x400;
}

u8
gpu_2d_read8(gpu_2d_engine *, u8 offset)
{
	LOG("2d engine read 8 at offset %02X\n", offset);
	return 0;
}

u16
gpu_2d_read16(gpu_2d_engine *gpu, u8 offset)
{
	switch (offset) {
	case 0x8:
		return gpu->bg_cnt[0];
	case 0xA:
		return gpu->bg_cnt[1];
	case 0xC:
		return gpu->bg_cnt[2];
	case 0xE:
		return gpu->bg_cnt[3];
	case 0x10:
		return gpu->bg_hofs[0];
	case 0x12:
		return gpu->bg_vofs[0];
	case 0x14:
		return gpu->bg_hofs[1];
	case 0x16:
		return gpu->bg_vofs[1];
	case 0x18:
		return gpu->bg_hofs[2];
	case 0x1A:
		return gpu->bg_vofs[2];
	case 0x1C:
		return gpu->bg_hofs[3];
	case 0x1E:
		return gpu->bg_vofs[3];
	case 0x44:
		return gpu->win_v[0];
	case 0x46:
		return gpu->win_v[1];
	case 0x48:
		return gpu->winin;
	case 0x4A:
		return gpu->winout;
	case 0x50:
		return gpu->bldcnt;
	case 0x52:
		return gpu->bldalpha;
	}

	LOG("2d engine read 16 at offset %02X\n", offset);
	return 0;
}

u32
gpu_2d_read32(gpu_2d_engine *gpu, u8 offset)
{
	switch (offset) {
	case 0x8:
		return (u32)gpu->bg_cnt[1] << 16 | gpu->bg_cnt[0];
	case 0xC:
		return (u32)gpu->bg_cnt[3] << 16 | gpu->bg_cnt[2];
	case 0x10:
		return (u32)gpu->bg_vofs[0] << 16 | gpu->bg_hofs[0];
	case 0x14:
		return (u32)gpu->bg_vofs[1] << 16 | gpu->bg_hofs[1];
	case 0x18:
		return (u32)gpu->bg_vofs[2] << 16 | gpu->bg_hofs[2];
	case 0x1C:
		return (u32)gpu->bg_vofs[3] << 16 | gpu->bg_hofs[3];
	case 0x48:
		return (u32)gpu->winout << 16 | gpu->winin;
	}

	LOG("2d engine read 32 at offset %02X\n", offset);
	return 0;
}

void
gpu_2d_write8(gpu_2d_engine *gpu, u8 offset, u8 value)
{
	if (!gpu->enabled) {
		return;
	}

	switch (offset) {
	case 0x40:
		gpu->win_h[0] = (gpu->win_h[0] & 0xFF00) | value;
		return;
	case 0x41:
		gpu->win_h[0] = (gpu->win_h[0] & 0x00FF) | (u16)value << 8;
		return;
	case 0x42:
		gpu->win_h[1] = (gpu->win_h[1] & 0xFF00) | value;
		return;
	case 0x43:
		gpu->win_h[1] = (gpu->win_h[1] & 0x00FF) | (u16)value << 8;
		return;
	case 0x44:
		gpu->win_v[0] = (gpu->win_v[0] & 0xFF00) | value;
		return;
	case 0x45:
		gpu->win_v[0] = (gpu->win_v[0] & 0x00FF) | (u16)value << 8;
		return;
	case 0x46:
		gpu->win_v[1] = (gpu->win_v[0] & 0xFF00) | value;
		return;
	case 0x47:
		gpu->win_v[1] = (gpu->win_v[0] & 0x00FF) | (u16)value << 8;
		return;
	case 0x4C:
		gpu->mosaic = (gpu->mosaic & 0xFF00) | value;
		return;
	case 0x4D:
		gpu->mosaic = (gpu->mosaic & 0x00FF) | (u16)value << 8;
		return;
	}

	LOG("2d engine write 8 to offset %08X\n", offset);
}

void
gpu_2d_write16(gpu_2d_engine *gpu, u8 offset, u16 value)
{
	if (!gpu->enabled) {
		return;
	}

	switch (offset) {
	case 0x8:
		gpu->bg_cnt[0] = value;
		return;
	case 0xA:
		gpu->bg_cnt[1] = value;
		return;
	case 0xC:
		gpu->bg_cnt[2] = value;
		return;
	case 0xE:
		gpu->bg_cnt[3] = value;
		return;
	case 0x10:
		gpu->bg_hofs[0] = value;
		return;
	case 0x12:
		gpu->bg_vofs[0] = value;
		return;
	case 0x14:
		gpu->bg_hofs[1] = value;
		return;
	case 0x16:
		gpu->bg_vofs[1] = value;
		return;
	case 0x18:
		gpu->bg_hofs[2] = value;
		return;
	case 0x1A:
		gpu->bg_vofs[2] = value;
		return;
	case 0x1C:
		gpu->bg_hofs[3] = value;
		return;
	case 0x1E:
		gpu->bg_vofs[3] = value;
		return;
	case 0x20:
		gpu->bg_pa[0] = value;
		return;
	case 0x22:
		gpu->bg_pb[0] = value;
		return;
	case 0x24:
		gpu->bg_pc[0] = value;
		return;
	case 0x26:
		gpu->bg_pd[0] = value;
		return;
	case 0x28:
		gpu->bg_ref_x_latch[0] &= ~0xFFFF;
		gpu->bg_ref_x_latch[0] |= value;
		gpu->bg_ref_x_reload[0] = true;
		return;
	case 0x2A:
		gpu->bg_ref_x_latch[0] &= 0xFFFF;
		gpu->bg_ref_x_latch[0] |= (u32)value << 16;
		gpu->bg_ref_x_reload[0] = true;
		return;
	case 0x2C:
		gpu->bg_ref_y_latch[0] &= ~0xFFFF;
		gpu->bg_ref_y_latch[0] |= value;
		gpu->bg_ref_y_reload[0] = true;
		return;
	case 0x2E:
		gpu->bg_ref_y_latch[0] &= 0xFFFF;
		gpu->bg_ref_y_latch[0] |= (u32)value << 16;
		gpu->bg_ref_y_reload[0] = true;
		return;
	case 0x30:
		gpu->bg_pa[1] = value;
		return;
	case 0x32:
		gpu->bg_pb[1] = value;
		return;
	case 0x34:
		gpu->bg_pc[1] = value;
		return;
	case 0x36:
		gpu->bg_pd[1] = value;
		return;
	case 0x38:
		gpu->bg_ref_x_latch[1] &= ~0xFFFF;
		gpu->bg_ref_x_latch[1] |= value;
		gpu->bg_ref_x_reload[1] = true;
		return;
	case 0x3A:
		gpu->bg_ref_x_latch[1] &= 0xFFFF;
		gpu->bg_ref_x_latch[1] |= (u32)value << 16;
		gpu->bg_ref_x_reload[1] = true;
		return;
	case 0x3C:
		gpu->bg_ref_y_latch[1] &= ~0xFFFF;
		gpu->bg_ref_y_latch[1] |= value;
		gpu->bg_ref_y_reload[1] = true;
		return;
	case 0x3E:
		gpu->bg_ref_y_latch[1] &= 0xFFFF;
		gpu->bg_ref_y_latch[1] |= (u32)value << 16;
		gpu->bg_ref_y_reload[1] = true;
		return;
	case 0x40:
		gpu->win_h[0] = value;
		return;
	case 0x42:
		gpu->win_h[1] = value;
		return;
	case 0x44:
		gpu->win_v[0] = value;
		return;
	case 0x46:
		gpu->win_v[1] = value;
		return;
	case 0x48:
		gpu->winin = value & 0x3F3F;
		return;
	case 0x4A:
		gpu->winout = value & 0x3F3F;
		return;
	case 0x4C:
		gpu->mosaic = value;
		return;
	case 0x50:
		gpu->bldcnt = value & 0x3FFF;
		return;
	case 0x52:
		gpu->bldalpha = value & 0x1F1F;
		return;
	case 0x54:
		gpu->bldy = value & 0x1F;
		return;
	}

	LOG("2d engine write 16 to offset %02X\n", offset);
}

void
gpu_2d_write32(gpu_2d_engine *gpu, u8 offset, u32 value)
{
	if (!gpu->enabled) {
		return;
	}

	switch (offset) {
	case 0x28:
		gpu->bg_ref_x_latch[0] = value;
		gpu->bg_ref_x_reload[0] = true;
		return;
	case 0x2C:
		gpu->bg_ref_y_latch[0] = value;
		gpu->bg_ref_y_reload[0] = true;
		return;
	case 0x38:
		gpu->bg_ref_x_latch[1] = value;
		gpu->bg_ref_x_reload[1] = true;
		return;
	case 0x3C:
		gpu->bg_ref_y_latch[1] = value;
		gpu->bg_ref_y_reload[1] = true;
		return;
	case 0x4C:
		gpu->mosaic = value;
		return;
	case 0x54:
		gpu->bldy = value & 0x1F;
		return;
	}

	gpu_2d_write16(gpu, offset, value);
	gpu_2d_write16(gpu, offset + 2, value >> 16);
}

void
gpu_on_scanline_start(nds_ctx *nds)
{
	s32 y = nds->vcount;
	gpu_2d_engine *gpu_a = &nds->gpu2d[0];
	gpu_2d_engine *gpu_b = &nds->gpu2d[1];

	check_window_y_bounds(gpu_a, y);
	check_window_y_bounds(gpu_b, y);

	if (y == 0 && gpu_a->dispcapcnt & BIT(31)) {
		gpu_a->display_capture = true;
	}

	if (y < 192) {
		render_scanline(gpu_a, y);
		render_scanline(gpu_b, y);
	}

	if (y == 192 && gpu_a->display_capture) {
		gpu_a->dispcapcnt &= ~BIT(31);
		gpu_a->display_capture = false;
	}
}

static void
check_window_y_bounds(gpu_2d_engine *gpu, u32 y)
{
	y &= 0xFF;

	for (int w = 0; w < 2; w++) {
		u8 y_start = gpu->win_v[w] >> 8;
		u8 y_end = gpu->win_v[w];

		if (y == y_start) {
			gpu->window_y_in_range[w] = true;
		}

		if (y == y_end) {
			gpu->window_y_in_range[w] = false;
		}
	}
}

} // namespace twice
