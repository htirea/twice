#include "nds/nds.h"

namespace twice {

event_scheduler::event_scheduler()
{
	events[HBLANK_START].cb = event_hblank_start;
	events[HBLANK_END].cb = event_hblank_end;
	arm_events[0][START_IMMEDIATE_DMAS].cb = event_start_immediate_dmas;
	arm_events[1][START_IMMEDIATE_DMAS].cb = event_start_immediate_dmas;
}

timestamp
get_next_event_time(nds_ctx *nds)
{
	auto& sc = nds->scheduler;

	timestamp time = sc.current_time + 64;

	for (int i = 0; i < event_scheduler::NUM_NDS_EVENTS; i++) {
		if (sc.events[i].enabled) {
			time = min_time(time, sc.events[i].time);
		}
	}

	for (int i = 0; i < event_scheduler::NUM_CPU_EVENTS; i++) {
		if (sc.arm_events[0][i].enabled) {
			time = min_time(time, sc.arm_events[0][i].time);
		}
		if (sc.arm_events[1][i].enabled) {
			time = min_time(time, sc.arm_events[1][i].time << 1);
		}
	}

	return time;
}

void
schedule_nds_event(nds_ctx *nds, int event, timestamp t)
{
	nds->scheduler.events[event].enabled = true;
	nds->scheduler.events[event].time = t << 1;
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

	if (cmp_time(event_time, nds->arm_target_cycles[cpuid]) < 0) {
		nds->arm_target_cycles[cpuid] = event_time;
	}

	nds->scheduler.arm_events[cpuid][event].enabled = true;
	nds->scheduler.arm_events[cpuid][event].time = event_time;
}

void
run_nds_events(nds_ctx *nds)
{
	auto& sc = nds->scheduler;

	for (int i = 0; i < event_scheduler::NUM_NDS_EVENTS; i++) {
		auto& event = sc.events[i];
		bool expired = cmp_time(sc.current_time, event.time) >= 0;
		if (event.enabled && expired) {
			event.enabled = false;
			if (event.cb) {
				event.cb(nds);
			}
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
		bool expired = cmp_time(cpu_time, event.time) >= 0;
		if (event.enabled && expired) {
			event.enabled = false;
			if (event.cb) {
				event.cb(nds, cpuid);
			}
		}
	}
}

} // namespace twice
