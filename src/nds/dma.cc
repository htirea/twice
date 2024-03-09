#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

static void start_dmas(nds_ctx *nds, int cpuid, int mode);
static void start_dma(nds_ctx *nds, int cpuid, int ch);
static void load_dmacnt_l(nds_ctx *nds, int cpuid, int ch);
static void load_dad(nds_ctx *nds, int cpuid, int ch);
template <int cpuid>
static void run_dma(nds_ctx *nds);
static void dma9_dmacnt_h_write(nds_ctx *nds, int ch, u16 value);
static void dma7_dmacnt_h_write(nds_ctx *nds, int ch, u16 value);
static void set_addr_step_and_width(nds_ctx *nds, int cpuid, int ch);

void
dma_controller_init(nds_ctx *nds, int cpuid)
{
	auto& dma = nds->dma[cpuid];
	dma.cycles = &nds->arm_cycles[cpuid];
	dma.target_cycles = &nds->arm_target_cycles[cpuid];
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

void
dmacnt_h_write(nds_ctx *nds, int cpuid, int ch, u16 value)
{
	if (cpuid == 0) {
		dma9_dmacnt_h_write(nds, ch, value);
	} else {
		dma7_dmacnt_h_write(nds, ch, value);
	}
}

void
dma_on_vblank(nds_ctx *nds)
{
	start_dmas(nds, 0, 1);
	start_dmas(nds, 1, 1);
}

void
dma_on_hblank_start(nds_ctx *nds)
{
	if (nds->vcount < 192) {
		start_dmas(nds, 0, 2);
	}
}

void
dma_on_scanline_start(nds_ctx *nds)
{
	if (nds->vcount < 192) {
		start_dmas(nds, 0, 3);
	}
}

void
event_start_immediate_dmas(nds_ctx *nds, intptr_t cpuid, timestamp)
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
	start_dmas(nds, cpuid, mode);
}

void
start_gxfifo_dmas(nds_ctx *nds)
{
	start_dmas(nds, 0, 7);
}

static void
start_dmas(nds_ctx *nds, int cpuid, int mode)
{
	auto& dma = nds->dma[cpuid];

	for (int ch = 0; ch < 4; ch++) {
		if (!dma.transfers[ch].enabled)
			continue;

		if (!(dma.transfers[ch].mode == mode))
			continue;

		start_dma(nds, cpuid, ch);
	}
}

static void
start_dma(nds_ctx *nds, int cpuid, int ch)
{
	auto& dma = nds->dma[cpuid];
	auto& t = dma.transfers[ch];

	if (t.repeat_reload) {
		t.repeat_reload = false;
		load_dmacnt_l(nds, cpuid, ch);
		if ((nds->dmacnt_h[cpuid][ch] >> 5 & 0x3) == 3) {
			load_dad(nds, cpuid, ch);
		}
	}

	dma.active |= BIT(ch);
}

static void
load_dmacnt_l(nds_ctx *nds, int cpuid, int ch)
{
	auto& word_count = nds->dma[cpuid].transfers[ch].word_count;

	if (cpuid == 0) {
		word_count = ((u32)nds->dmacnt_h[0][ch] << 16 |
				nds->dmacnt_l[0][ch]);
		word_count &= MASK(21);

		if (word_count == 0) {
			word_count = 0x200000;
		}
	} else {
		if (ch == 3) {
			word_count = nds->dmacnt_l[1][ch] & MASK(16);

			if (word_count == 0) {
				word_count = 0x10000;
			}
		} else {
			word_count = nds->dmacnt_l[1][ch] & MASK(14);

			if (word_count == 0) {
				word_count = 0x4000;
			}
		}
	}
}

static void
load_dad(nds_ctx *nds, int cpuid, int ch)
{
	auto& dad = nds->dma[cpuid].transfers[ch].dad;

	if (cpuid == 0) {
		dad = nds->dma_dad[0][ch] & MASK(28);
	} else {
		if (ch == 3) {
			dad = nds->dma_dad[1][ch] & MASK(28);
		} else {
			dad = nds->dma_dad[1][ch] & MASK(27);
		}
	}
}

template <int cpuid>
static void
run_dma(nds_ctx *nds)
{
	auto& dma = nds->dma[cpuid];
	int ch = std::countr_zero(dma.active);
	auto& t = dma.transfers[ch];

	while (t.count < t.word_count && *dma.cycles < *dma.target_cycles) {
		if (t.word_width == 4) {
			u32 value = bus_read<cpuid, u32>(nds, t.sad);
			bus_write<cpuid, u32>(nds, t.dad, value);
		} else {
			u16 value = bus_read<cpuid, u16>(nds, t.sad);
			bus_write<cpuid, u16>(nds, t.dad, value);
		}

		t.count += 1;
		t.sad += t.sad_step;
		t.dad += t.dad_step;
		*dma.cycles += 2;
		dma.cycles_executed += 2;
	}

	if (t.count == t.word_count) {
		auto& dmacnt = nds->dmacnt_h[cpuid][ch];

		if (dmacnt & BIT(14)) {
			request_interrupt(nds->cpu[cpuid], 8 + ch);
		}

		if (dmacnt & BIT(9)) {
			t.repeat_reload = true;
		} else {
			dmacnt &= ~BIT(15);
			t.enabled = false;
		}

		t.count = 0;
		dma.active &= ~BIT(ch);
	}
}

static void
dma9_dmacnt_h_write(nds_ctx *nds, int ch, u16 value)
{
	auto& dma = nds->dma[0];
	auto& dmacnt = nds->dmacnt_h[0][ch];
	auto& t = dma.transfers[ch];

	bool old_enabled = dmacnt & BIT(15);
	dmacnt = value;
	t.enabled = value & BIT(15);
	t.mode = value >> 11 & 0x7;
	set_addr_step_and_width(nds, 0, ch);

	if (!old_enabled && t.enabled) {
		t.sad = nds->dma_sad[0][ch] & MASK(28);

		load_dad(nds, 0, ch);
		load_dmacnt_l(nds, 0, ch);

		switch (t.mode) {
		case 0:
			dma.requested_imm_dmas |= BIT(ch);
			schedule_event_after(nds, 0,
					scheduler::ARM9_IMMEDIATE_DMA, 2);
			break;
		case 7:
			if (nds->gpu3d.fifo.buffer.size() < 128) {
				dma.active |= BIT(ch);
			}
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
}

static void
dma7_dmacnt_h_write(nds_ctx *nds, int ch, u16 value)
{
	auto& dma = nds->dma[1];
	auto& dmacnt = nds->dmacnt_h[1][ch];
	auto& t = dma.transfers[ch];

	bool old_enabled = dmacnt & BIT(15);
	dmacnt = value;
	t.enabled = value & BIT(15);
	t.mode = value >> 12 & 0x3;
	set_addr_step_and_width(nds, 1, ch);

	if (!old_enabled && t.enabled) {
		if (ch == 0) {
			t.sad = nds->dma_sad[1][ch] & MASK(27);
		} else {
			t.sad = nds->dma_sad[1][ch] & MASK(28);
		}

		load_dad(nds, 1, ch);
		load_dmacnt_l(nds, 1, ch);

		switch (t.mode) {
		case 0:
			dma.requested_imm_dmas |= BIT(ch);
			schedule_event_after(nds, 1,
					scheduler::ARM7_IMMEDIATE_DMA, 4);
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
}

static void
set_addr_step_and_width(nds_ctx *nds, int cpuid, int ch)
{
	auto& t = nds->dma[cpuid].transfers[ch];
	auto& dmacnt = nds->dmacnt_h[cpuid][ch];

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

} // namespace twice
