#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/mem/vram.h"

namespace twice {

void gpu_draw_scanline(NDS *nds);

void
gpu_on_vblank(NDS *nds)
{
	nds->dispstat[0] |= BIT(0);
	nds->dispstat[1] |= BIT(0);

	if (nds->dispstat[0] & BIT(3)) {
		nds->cpu[0]->request_interrupt(0);
	}

	if (nds->dispstat[1] & BIT(3)) {
		nds->cpu[1]->request_interrupt(0);
	}

	dma_on_vblank(nds);

	nds->frame_finished = true;
}

void
gpu_on_hblank_start(NDS *nds)
{
	nds->dispstat[0] |= BIT(1);
	nds->dispstat[1] |= BIT(1);

	if (nds->dispstat[0] & BIT(4)) {
		nds->cpu[0]->request_interrupt(1);
	}

	if (nds->dispstat[1] & BIT(4)) {
		nds->cpu[1]->request_interrupt(1);
	}

	if (nds->gpu.ly < 192) {
		dma_on_hblank_start(nds);
	}

	nds->scheduler.reschedule_event_after(Scheduler::HBLANK_START, 2130);
}

void
gpu_on_hblank_end(NDS *nds)
{
	auto& gpu = nds->gpu;

	gpu.ly += 1;
	if (gpu.ly == 263) {
		gpu.ly = 0;
	}

	if (gpu.ly < 192) {
		gpu_draw_scanline(nds);
	} else if (gpu.ly == 192) {
		gpu_on_vblank(nds);
	} else if (gpu.ly == 262) {
		/* vblank flag isn't set in last scanline */
		nds->dispstat[0] &= ~BIT(0);
		nds->dispstat[1] &= ~BIT(0);
	}

	/* TODO: check LYC */

	nds->scheduler.reschedule_event_after(Scheduler::HBLANK_END, 2130);
}

u32
ABGR1555_TO_ABGR8888(u16 color)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;
	u8 a = color >> 15;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	a = a * 0xFF;

	return (a << 24) | (b << 16) | (g << 8) | r;
}

void
gpu_draw_scanline(NDS *nds)
{
	u32 start = nds->gpu.ly * NDS_FB_W;
	u32 end = start + NDS_FB_W;

	for (u32 i = start; i < end; i++) {
		nds->fb[i] = ABGR1555_TO_ABGR8888(
				vram_read_lcdc<u16>(nds, i * 2));
	}
}

} // namespace twice
