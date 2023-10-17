#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/spi.h"

#include "libtwice/exception.h"

namespace twice {

event_scheduler::event_scheduler()
{
	events[HBLANK_START].cb = event_hblank_start;
	events[HBLANK_END].cb = event_hblank_end;
	events[ROM_ADVANCE_TRANSFER].cb = event_advance_rom_transfer;
	events[AUXSPI_TRANSFER_COMPLETE].cb = event_auxspi_transfer_complete;
	events[SAMPLE_AUDIO].cb = event_sample_audio;
	arm_events[0][START_IMMEDIATE_DMAS].cb = event_start_immediate_dmas;
	arm_events[1][START_IMMEDIATE_DMAS].cb = event_start_immediate_dmas;
	for (int i = 0; i < 4; i++) {
		arm_events[0][TIMER0_OVERFLOW + i].cb = event_timer_overflow;
		arm_events[0][TIMER0_OVERFLOW + i].data = i;
		arm_events[1][TIMER0_OVERFLOW + i].cb = event_timer_overflow;
		arm_events[1][TIMER0_OVERFLOW + i].data = i;
	}
	arm_events[1][SPI_TRANSFER_COMPLETE].cb = event_spi_transfer_complete;
}

timestamp
get_next_event_time(nds_ctx *nds)
{
	auto& sc = nds->scheduler;

	timestamp curr = sc.current_time;
	timestamp limit = sc.current_time + 64;
	timestamp target = -1;

	for (int i = 0; i < event_scheduler::NUM_NDS_EVENTS; i++) {
		if (sc.events[i].enabled) {
			target = std::min(target, sc.events[i].time);
		}
	}

	for (int i = 0; i < event_scheduler::NUM_CPU_EVENTS; i++) {
		if (sc.arm_events[0][i].enabled) {
			target = std::min(target, sc.arm_events[0][i].time);
		}
		if (sc.arm_events[1][i].enabled) {
			target = std::min(
					target, sc.arm_events[1][i].time << 1);
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
schedule_nds_event(nds_ctx *nds, int event, timestamp t)
{
	nds->scheduler.events[event].enabled = true;
	nds->scheduler.events[event].time = t << 1;
}

void
schedule_nds_event_after(nds_ctx *nds, int cpuid, int event, timestamp dt)
{
	if (cpuid == 0) {
		dt <<= 1;
	}

	timestamp event_time = nds->arm_cycles[cpuid] + dt;

	if (event_time < nds->arm_target_cycles[cpuid]) {
		nds->arm_target_cycles[cpuid] = event_time;
	}

	nds->scheduler.events[event].enabled = true;
	nds->scheduler.events[event].time = event_time;
}

void
reschedule_nds_event_after(nds_ctx *nds, int event, timestamp dt)
{
	nds->scheduler.events[event].enabled = true;
	nds->scheduler.events[event].time += dt << 1;
}

void
schedule_cpu_event_after(nds_ctx *nds, int cpuid, int event, timestamp dt)
{
	if (cpuid == 0) {
		dt <<= 1;
	}

	timestamp event_time = nds->arm_cycles[cpuid] + dt;

	if (event_time < nds->arm_target_cycles[cpuid]) {
		nds->arm_target_cycles[cpuid] = event_time;
	}

	nds->scheduler.arm_events[cpuid][event].enabled = true;
	nds->scheduler.arm_events[cpuid][event].time = event_time;
}

void
cancel_cpu_event(nds_ctx *nds, int cpuid, int event)
{
	nds->scheduler.arm_events[cpuid][event].enabled = false;
}

void
run_nds_events(nds_ctx *nds)
{
	auto& sc = nds->scheduler;

	for (int i = 0; i < event_scheduler::NUM_NDS_EVENTS; i++) {
		auto& event = sc.events[i];
		bool expired = sc.current_time >= event.time;
		if (event.enabled && expired) {
			event.enabled = false;
			event.cb(nds);
		}
	}
}

void
run_cpu_events(nds_ctx *nds, int cpuid)
{
	auto& sc = nds->scheduler;
	timestamp cpu_time = nds->arm_cycles[cpuid];

	for (int i = 0; i < event_scheduler::NUM_CPU_EVENTS; i++) {
		auto& event = sc.arm_events[cpuid][i];
		bool expired = cpu_time >= event.time;
		if (event.enabled && expired) {
			event.enabled = false;
			event.cb(nds, cpuid, event.data);
		}
	}
}

} // namespace twice
