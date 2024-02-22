#ifndef TWICE_DMA_H
#define TWICE_DMA_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct dma_transfer_state {
	bool enabled{};
	int mode{};
	u32 sad{};
	u32 dad{};
	u32 word_count{};
	u32 count{};
	int word_width{};
	int sad_step{};
	int dad_step{};
	bool repeat_reload{};
};

struct dma_controller {
	u32 active{};
	u32 requested_imm_dmas{};
	dma_transfer_state transfers[4];
	timestamp *target_cycles{};
	timestamp *cycles{};
	u64 cycles_executed{};
};

void dma_controller_init(nds_ctx *nds, int cpuid);
void run_dma9(nds_ctx *nds);
void run_dma7(nds_ctx *nds);
void dmacnt_h_write(nds_ctx *nds, int cpuid, int channel, u16 value);
void dma_on_vblank(nds_ctx *nds);
void dma_on_hblank_start(nds_ctx *nds);
void dma_on_scanline_start(nds_ctx *nds);
void event_start_immediate_dmas(nds_ctx *nds, intptr_t data, timestamp late);
void start_cartridge_dmas(nds_ctx *nds, int cpuid);
void start_gxfifo_dmas(nds_ctx *nds);

} // namespace twice

#endif
