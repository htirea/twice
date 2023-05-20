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
};

struct Dma {
	Dma(NDS *nds, int cpuid);

	u32 active{};
	DmaTransfer transfers[4];

	NDS *nds{};
	int cpuid;

	u64& target_cycles;
	u64& cycles;

	virtual void run() = 0;
	virtual void dmacnt_h_write(int channel, u16 value) = 0;

	void start_transfer(int channel);
	void set_addr_step_and_width(int channel, u16 dmacnt);
};

struct Dma9 final : Dma {
	Dma9(NDS *nds);

	void run() override;
	void dmacnt_h_write(int channel, u16 value) override;

	void load_dad(int channel);
	void load_dmacnt_l(int channel);
};

struct Dma7 final : Dma {
	Dma7(NDS *nds);

	void run() override;
	void dmacnt_h_write(int channel, u16 value) override;

	void load_dad(int channel);
	void load_dmacnt_l(int channel);
};

void dma_on_vblank(NDS *nds);
void dma_on_hblank_start(NDS *nds);

} // namespace twice

#endif
