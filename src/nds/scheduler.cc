#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/spi.h"

#include "libtwice/exception.h"

namespace twice {

static void run_events(
		nds_ctx *nds, u32 start, u32 length, timestamp curr_time);

scheduler::scheduler()
{
	callbacks[HBLANK_START] = event_hblank_start;
	callbacks[HBLANK_END] = event_hblank_end;
	callbacks[SAMPLE_AUDIO] = event_sample_audio;
	callbacks[CART_TRANSFER] = event_advance_rom_transfer;
	callbacks[AUXSPI_TRANSFER] = event_auxspi_transfer_complete;

	callbacks[ARM9_TIMER0_OVERFLOW] = event_timer_overflow;
	callbacks[ARM9_TIMER1_OVERFLOW] = event_timer_overflow;
	callbacks[ARM9_TIMER2_OVERFLOW] = event_timer_overflow;
	callbacks[ARM9_TIMER3_OVERFLOW] = event_timer_overflow;
	callbacks[ARM9_IMMEDIATE_DMA] = event_start_immediate_dmas;
	data[ARM9_TIMER0_OVERFLOW] = 0;
	data[ARM9_TIMER1_OVERFLOW] = 1;
	data[ARM9_TIMER2_OVERFLOW] = 2;
	data[ARM9_TIMER3_OVERFLOW] = 3;
	data[ARM9_IMMEDIATE_DMA] = 0;

	callbacks[ARM7_TIMER0_OVERFLOW] = event_timer_overflow;
	callbacks[ARM7_TIMER1_OVERFLOW] = event_timer_overflow;
	callbacks[ARM7_TIMER2_OVERFLOW] = event_timer_overflow;
	callbacks[ARM7_TIMER3_OVERFLOW] = event_timer_overflow;
	callbacks[ARM7_IMMEDIATE_DMA] = event_start_immediate_dmas;
	callbacks[ARM7_SPI_TRANSFER] = event_spi_transfer_complete;
	data[ARM7_TIMER0_OVERFLOW] = 4;
	data[ARM7_TIMER1_OVERFLOW] = 5;
	data[ARM7_TIMER2_OVERFLOW] = 6;
	data[ARM7_TIMER3_OVERFLOW] = 7;
	data[ARM7_IMMEDIATE_DMA] = 1;
}

timestamp
get_next_event_time(nds_ctx *nds)
{
	auto& sc = nds->sc;

	timestamp curr = nds->arm_cycles[0];
	timestamp limit = curr + 64;
	timestamp target = -1;

	for (int i = 0; i < scheduler::NUM_EVENTS; i++) {
		if (sc.enabled[i]) {
			target = std::min(target, sc.expiry[i]);
		}
	}

	bool fast_skip = !nds->dma[0].active && !nds->dma[1].active &&
	                 nds->cpu[0]->halted && nds->cpu[1]->halted;
	if (!fast_skip) {
		target = std::min(target, limit);
	}

	if (target == curr) {
		throw twice_error("target timestamp == curr");
	}

	return target;
}

void
schedule_event_after(nds_ctx *nds, int id, timestamp dt)
{
	auto& sc = nds->sc;
	sc.expiry[id] += dt;
	sc.enabled[id] = true;
}

void
schedule_event_after(nds_ctx *nds, int cpuid, int id, timestamp dt)
{
	auto& sc = nds->sc;
	timestamp t = (nds->arm_cycles[cpuid] << cpuid) + dt;

	if (t < nds->arm_target_cycles[cpuid] << cpuid) {
		nds->arm_target_cycles[cpuid] = t >> cpuid;
	}

	sc.enabled[id] = true;
	sc.expiry[id] = t;
}

void
cancel_event(nds_ctx *nds, int id)
{
	auto& sc = nds->sc;
	sc.enabled[id] = false;
}

void
run_system_events(nds_ctx *nds)
{
	run_events(nds, scheduler::HBLANK_START, 5, nds->arm_cycles[0]);
}

void
run_cpu_events(nds_ctx *nds, int cpuid)
{
	if (cpuid == 0) {
		run_events(nds, scheduler::ARM9_TIMER0_OVERFLOW, 5,
				nds->arm_cycles[0]);
	} else {
		run_events(nds, scheduler::ARM7_TIMER0_OVERFLOW, 6,
				nds->arm_cycles[1] << 1);
	}
}

int
get_timer_overflow_event_id(int cpuid, int timer_id)
{
	int id = cpuid == 0 ? scheduler::ARM9_TIMER0_OVERFLOW
	                    : scheduler::ARM7_TIMER0_OVERFLOW;
	return id + timer_id;
}

static void
run_events(nds_ctx *nds, u32 start, u32 length, timestamp curr_time)
{
	auto& sc = nds->sc;

	for (u32 i = start; i < start + length; i++) {
		if (!sc.enabled[i])
			continue;

		if (curr_time >= sc.expiry[i]) {
			timestamp late = curr_time - sc.expiry[i];
			sc.enabled[i] = false;
			sc.callbacks[i](nds, sc.data[i], late);
		}
	}
}

} // namespace twice
