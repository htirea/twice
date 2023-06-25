#ifndef TWICE_DMA_H
#define TWICE_DMA_H

#include "common/types.h"

namespace twice {

struct NDS;

struct DmaTransfer {
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

struct Dma {
	Dma(NDS *nds, int cpuid);

	u32 active{};
	u32 requested_imm_dmas{};
	DmaTransfer transfers[4];

	NDS *nds{};
	int cpuid;

	u64& target_cycles;
	u64& cycles;

	virtual void dmacnt_h_write(int channel, u16 value) = 0;
};

struct Dma9 final : Dma {
	Dma9(NDS *nds);

	void dmacnt_h_write(int channel, u16 value) override;
};

struct Dma7 final : Dma {
	Dma7(NDS *nds);

	void dmacnt_h_write(int channel, u16 value) override;
};

void run_dma9(NDS *nds);
void run_dma7(NDS *nds);

void dma_on_vblank(NDS *nds);
void dma_on_hblank_start(NDS *nds);
void event_start_immediate_dmas(NDS *nds, int cpuid);

} // namespace twice

#endif
