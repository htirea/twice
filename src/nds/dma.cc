#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

dma_controller::dma_controller(nds_ctx *nds, int cpuid)
	: target_cycles(nds->arm_target_cycles[cpuid]),
	  cycles(nds->arm_cycles[cpuid])
{
}

template <int cpuid>
static void
run_dma(nds_ctx *nds)
{
	auto& dma = nds->dma[cpuid];
	int channel = std::countr_zero(dma.active);
	auto& t = dma.transfers[channel];

	int width = t.word_width;

	while (t.count < t.word_count &&
			cmp_time(dma.cycles, dma.target_cycles) < 0) {
		if (width == 4) {
			u32 value = bus_read<cpuid, u32>(nds, t.sad);
			bus_write<cpuid, u32>(nds, t.dad, value);
		} else {
			u16 value = bus_read<cpuid, u16>(nds, t.sad);
			bus_write<cpuid, u16>(nds, t.dad, value);
		}

		t.sad += t.sad_step;
		t.dad += t.dad_step;
		dma.cycles += 2;

		t.count += 1;
	}

	if (t.count == t.word_count) {
		auto& dmacnt = nds->dmacnt_h[cpuid][channel];

		if (dmacnt & BIT(14)) {
			request_interrupt(nds->cpu[cpuid], 8 + channel);
		}

		if (dmacnt & BIT(9)) {
			t.repeat_reload = true;
		} else {
			dmacnt &= ~BIT(15);
			t.enabled = false;
		}

		t.count = 0;
		dma.active &= ~BIT(channel);
	}
}

void
run_dma9(nds_ctx *nds)
{
	run_dma<0>(nds);
}

void
run_dma7(nds_ctx *nds)
{
	run_dma<1>(nds);
}

static void
set_addr_step_and_width(dma_controller *dma, int channel, u16 dmacnt)
{
	auto& t = dma->transfers[channel];

	switch (dmacnt >> 5 & 0x3) {
	case 0:
		t.dad_step = 1;
		break;
	case 1:
		t.dad_step = -1;
		break;
	case 2:
		t.dad_step = 0;
		break;
	case 3:
		t.dad_step = 1;
		break;
	}

	switch (dmacnt >> 7 & 0x3) {
	case 0:
		t.sad_step = 1;
		break;
	case 1:
		t.sad_step = -1;
		break;
	case 2:
		t.sad_step = 0;
		break;
	case 3:
		throw twice_error("dma invalid source adr step");
	}

	if (dmacnt & BIT(10)) {
		t.word_width = 4;
	} else {
		t.word_width = 2;
	}

	t.sad_step *= t.word_width;
	t.dad_step *= t.word_width;
}

static void
load_dad(nds_ctx *nds, int cpuid, int channel)
{
	auto& dad = nds->dma[cpuid].transfers[channel].dad;

	if (cpuid == 0) {
		dad = nds->dma_dad[0][channel] & MASK(28);
	} else {
		if (channel == 3) {
			dad = nds->dma_dad[1][channel] & MASK(28);
		} else {
			dad = nds->dma_dad[1][channel] & MASK(27);
		}
	}
}

static void
load_dmacnt_l(nds_ctx *nds, int cpuid, int channel)
{
	auto& word_count = nds->dma[cpuid].transfers[channel].word_count;

	if (cpuid == 0) {
		word_count = nds->dmacnt_l[0][channel] & MASK(21);

		if (word_count == 0) {
			word_count = 0x200000;
		}
	} else {
		if (channel == 3) {
			word_count = nds->dmacnt_l[1][channel] & MASK(16);

			if (word_count == 0) {
				word_count = 0x10000;
			}
		} else {
			word_count = nds->dmacnt_l[1][channel] & MASK(14);

			if (word_count == 0) {
				word_count = 0x4000;
			}
		}
	}
}

static void
dma9_dmacnt_h_write(nds_ctx *nds, int channel, u16 value)
{
	auto& dma = nds->dma[0];
	auto& dmacnt = nds->dmacnt_h[0][channel];
	auto& t = dma.transfers[channel];

	bool old_enabled = dmacnt & BIT(15);
	t.enabled = value & BIT(15);
	t.mode = value >> 11 & 0x7;
	set_addr_step_and_width(&dma, channel, value);

	if (!old_enabled && t.enabled) {
		t.sad = nds->dma_sad[0][channel] & MASK(28);

		load_dad(nds, 0, channel);
		load_dmacnt_l(nds, 0, channel);

		switch (t.mode) {
		case 0:
			dma.requested_imm_dmas |= BIT(channel);
			schedule_cpu_event_after(nds, 0,
					event_scheduler::START_IMMEDIATE_DMAS,
					1);
			break;
		case 1:
		case 2:
		case 3:
		case 5:
			break;
		default:
			throw twice_error("arm9 dma mode not implemented");
		}

		t.repeat_reload = false;
		t.count = 0;
	}

	dmacnt = value;
}

static void
dma7_dmacnt_h_write(nds_ctx *nds, int channel, u16 value)
{
	auto& dma = nds->dma[1];
	auto& dmacnt = nds->dmacnt_h[1][channel];
	auto& t = dma.transfers[channel];

	bool old_enabled = dmacnt & BIT(15);
	t.enabled = value & BIT(15);
	t.mode = value >> 12 & 0x3;
	set_addr_step_and_width(&dma, channel, value);

	if (!old_enabled && t.enabled) {
		if (channel == 0) {
			t.sad = nds->dma_sad[1][channel] & MASK(27);
		} else {
			t.sad = nds->dma_sad[1][channel] & MASK(28);
		}

		load_dad(nds, 1, channel);
		load_dmacnt_l(nds, 1, channel);

		switch (t.mode) {
		case 0:
			dma.requested_imm_dmas |= BIT(channel);
			schedule_cpu_event_after(nds, 1,
					event_scheduler::START_IMMEDIATE_DMAS,
					2);
			break;
		case 1:
		case 2:
			break;
		case 3:
			throw twice_error("arm7 dma mode 3 not implemented");
		}

		t.repeat_reload = false;
		t.count = 0;
	}

	dmacnt = value;
}

void
dmacnt_h_write(nds_ctx *nds, int cpuid, int channel, u16 value)
{
	if (cpuid == 0) {
		dma9_dmacnt_h_write(nds, channel, value);
	} else {
		dma7_dmacnt_h_write(nds, channel, value);
	}
}

static void
start_transfer(nds_ctx *nds, int cpuid, int channel)
{
	auto& dma = nds->dma[cpuid];
	auto& t = dma.transfers[channel];

	if (t.repeat_reload) {
		t.repeat_reload = false;
		load_dmacnt_l(nds, cpuid, channel);
		if ((nds->dmacnt_h[cpuid][channel] >> 5 & 0x3) == 3) {
			load_dad(nds, cpuid, channel);
		}
	}

	dma.active |= BIT(channel);
}

void
dma_on_vblank(nds_ctx *nds)
{
	for (int channel = 0; channel < 4; channel++) {
		if (nds->dma[0].transfers[channel].enabled) {
			if (nds->dma[0].transfers[channel].mode == 1) {
				start_transfer(nds, 0, channel);
			}
		}

		if (nds->dma[1].transfers[channel].enabled) {
			if (nds->dma[1].transfers[channel].mode == 1) {
				start_transfer(nds, 1, channel);
			}
		}
	}
}

void
dma_on_hblank_start(nds_ctx *nds)
{
	if (nds->vcount >= 192) {
		return;
	}

	for (int channel = 0; channel < 4; channel++) {
		if (nds->dma[0].transfers[channel].enabled) {
			if (nds->dma[0].transfers[channel].mode == 2) {
				start_transfer(nds, 0, channel);
			}
		}
	}
}

void
dma_on_scanline_start(nds_ctx *nds)
{
	if (nds->vcount >= 192) {
		return;
	}

	for (int channel = 0; channel < 4; channel++) {
		if (nds->dma[0].transfers[channel].enabled) {
			if (nds->dma[0].transfers[channel].mode == 3) {
				start_transfer(nds, 0, channel);
			}
		}
	}
}

void
event_start_immediate_dmas(nds_ctx *nds, int cpuid, intptr_t)
{
	auto& dma = nds->dma[cpuid];

	/* TODO: don't start all requested dmas */
	dma.active |= dma.requested_imm_dmas;
	dma.requested_imm_dmas = 0;
}

void
start_cartridge_dmas(nds_ctx *nds, int cpuid)
{
	int mode = cpuid == 0 ? 5 : 2;

	for (int channel = 0; channel < 4; channel++) {
		if (nds->dma[cpuid].transfers[channel].enabled) {
			if (nds->dma[cpuid].transfers[channel].mode == mode) {
				start_transfer(nds, cpuid, channel);
			}
		}
	}
}

} // namespace twice
