#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

Dma::Dma(NDS *nds, int cpuid)
	: nds(nds),
	  cpuid(cpuid),
	  target_cycles(nds->arm_target_cycles[cpuid]),
	  cycles(nds->arm_cycles[cpuid])
{
}

Dma9::Dma9(NDS *nds)
	: Dma(nds, 0)
{
}

Dma7::Dma7(NDS *nds)
	: Dma(nds, 1)
{
}

void
Dma::start_transfer(int channel)
{
	auto& t = transfers[channel];

	t.count = 0;
	active |= BIT(channel);
}

void
Dma9::run()
{
	int channel = std::countr_zero(active);
	auto& t = transfers[channel];

	int width = t.word_width;

	while (t.count < t.word_count && cycles < target_cycles) {
		if (width == 4) {
			auto value = bus9_read<u32>(nds, t.sad);
			bus9_write<u32>(nds, t.dad, value);
		} else {
			auto value = bus9_read<u16>(nds, t.sad);
			bus9_write<u16>(nds, t.dad, value);
		}

		t.sad += t.sad_step;
		t.dad += t.dad_step;
		cycles += 2;

		t.count += 1;
	}

	if (t.count == t.word_count) {
		auto& dmacnt = nds->dmacnt_h[0][channel];

		if (dmacnt & BIT(14)) {
			nds->cpu[0]->request_interrupt(8 + channel);
		}

		if (dmacnt & BIT(9)) {
			/* TODO: should this happen on dma start? */
			load_dmacnt_l(channel);
			if ((dmacnt >> 5 & 0x3) == 3) {
				load_dad(channel);
			}

		} else {
			dmacnt &= ~BIT(15);
			t.enabled = false;
		}

		active &= ~BIT(channel);
	}
}

void
Dma7::run()
{
	int channel = std::countr_zero(active);
	auto& t = transfers[channel];

	int width = t.word_width;

	while (t.count < t.word_count && cycles < target_cycles) {
		if (width == 4) {
			auto value = bus7_read<u32>(nds, t.sad);
			bus7_write<u32>(nds, t.dad, value);
		} else {
			auto value = bus7_read<u16>(nds, t.sad);
			bus7_write<u16>(nds, t.dad, value);
		}

		t.sad += t.sad_step;
		t.dad += t.dad_step;
		cycles += 2;

		t.count += 1;
	}

	if (t.count == t.word_count) {
		auto& dmacnt = nds->dmacnt_h[1][channel];

		if (dmacnt & BIT(14)) {
			nds->cpu[1]->request_interrupt(8 + channel);
		}

		if (dmacnt & BIT(9)) {
			/* TODO: should this happen on dma start? */
			load_dmacnt_l(channel);
			if ((dmacnt >> 5 & 0x3) == 3) {
				load_dad(channel);
			}
		} else {
			dmacnt &= ~BIT(15);
			t.enabled = false;
		}

		active &= ~BIT(channel);
	}
}

void
Dma::set_addr_step_and_width(int channel, u16 dmacnt)
{
	auto& t = transfers[channel];

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
		throw TwiceError("dma invalid source adr step");
	}

	if (dmacnt & BIT(10)) {
		t.word_width = 4;
	} else {
		t.word_width = 2;
	}

	t.sad_step *= t.word_width;
	t.dad_step *= t.word_width;
}

void
Dma9::dmacnt_h_write(int channel, u16 value)
{
	auto& dmacnt = nds->dmacnt_h[0][channel];
	auto& t = transfers[channel];

	bool old_enabled = dmacnt & BIT(15);
	t.enabled = value & BIT(15);
	t.mode = value >> 11 & 0x7;
	set_addr_step_and_width(channel, value);

	if (!old_enabled && t.enabled) {
		t.sad = nds->dma_sad[0][channel] & MASK(28);

		load_dad(channel);
		load_dmacnt_l(channel);

		switch (t.mode) {
		case 0:
			/* TODO: the event cb should call the start function */
			t.count = 0;
			active |= BIT(channel);
			schedule_arm_event_after(
					nds, 0, Scheduler::NULL_EVENT, 1);
			break;
		case 1:
		case 2:
			break;
		default:
			throw TwiceError("arm9 dma mode not implemented");
		}
	}

	dmacnt = value;
}

void
Dma7::dmacnt_h_write(int channel, u16 value)
{
	auto& dmacnt = nds->dmacnt_h[1][channel];
	auto& t = transfers[channel];

	bool old_enabled = dmacnt & BIT(15);
	t.enabled = value & BIT(15);
	t.mode = value >> 12 & 0x3;
	set_addr_step_and_width(channel, value);

	if (!old_enabled && t.enabled) {
		if (channel == 0) {
			t.sad = nds->dma_sad[1][channel] & MASK(27);
		} else {
			t.sad = nds->dma_sad[1][channel] & MASK(28);
		}

		load_dad(channel);
		load_dmacnt_l(channel);

		switch (t.mode) {
		case 0:
			/* TODO: the event cb should call the start function */
			t.count = 0;
			active |= BIT(channel);
			schedule_arm_event_after(
					nds, 1, Scheduler::NULL_EVENT, 2);
			break;
		case 1:
			break;
		case 2:
			throw TwiceError("arm7 dma mode 2 not implemented");
		case 3:
			throw TwiceError("arm7 dma mode 3 not implemented");
		}
	}

	dmacnt = value;
}

void
Dma9::load_dad(int channel)
{
	transfers[channel].dad = nds->dma_dad[0][channel] & MASK(28);
}

void
Dma7::load_dad(int channel)
{
	if (channel == 3) {
		transfers[channel].dad = nds->dma_dad[1][channel] & MASK(28);
	} else {
		transfers[channel].dad = nds->dma_dad[1][channel] & MASK(27);
	}
}

void
Dma9::load_dmacnt_l(int channel)
{
	u32 word_count = nds->dmacnt_l[0][channel] & MASK(21);

	if (word_count == 0) {
		word_count = 0x200000;
	}

	transfers[channel].word_count = word_count;
}

void
Dma7::load_dmacnt_l(int channel)
{
	u32 word_count;

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

	transfers[channel].word_count = word_count;
}

void
dma_on_vblank(NDS *nds)
{
	for (int channel = 0; channel < 4; channel++) {
		if (nds->dma[0]->transfers[channel].enabled) {
			if (nds->dma[0]->transfers[channel].mode == 1) {
				nds->dma[0]->start_transfer(channel);
			}
		}

		if (nds->dma[1]->transfers[channel].enabled) {
			if (nds->dma[1]->transfers[channel].mode == 1) {
				nds->dma[1]->start_transfer(channel);
			}
		}
	}
}

void
dma_on_hblank_start(NDS *nds)
{
	if (nds->vcount >= 192) {
		return;
	}

	for (int channel = 0; channel < 4; channel++) {
		if (nds->dma[0]->transfers[channel].enabled) {
			if (nds->dma[0]->transfers[channel].mode == 2) {
				nds->dma[0]->start_transfer(channel);
			}
		}
	}
}

} // namespace twice
