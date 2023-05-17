#include "nds/nds.h"

namespace twice {

Scheduler::Scheduler()
{
	events[HBLANK_START].cb = gpu_on_hblank_start;
	events[HBLANK_END].cb = gpu_on_hblank_end;
}

void
Scheduler::schedule_event(int event, u64 t)
{
	events[event].enabled = true;
	events[event].time = t;
}

void
Scheduler::reschedule_event_after(int event, u64 dt)
{
	events[event].enabled = true;
	events[event].time += dt;
}

u64
Scheduler::get_next_event_time()
{
	u64 time = current_time + 32;

	for (int i = 0; i < NUM_EVENTS; i++) {
		if (events[i].enabled) {
			time = std::min(time, events[i].time);
		}
	}

	for (int i = 0; i < NUM_ARM_EVENTS; i++) {
		if (arm_events[0][i].enabled) {
			time = std::min(time, arm_events[0][i].time >> 1);
		}
		if (arm_events[1][i].enabled) {
			time = std::min(time, arm_events[1][i].time);
		}
	}

	return time;
}

void
schedule_arm_event_after(NDS *nds, int cpuid, int event, u64 dt)
{
	if (cpuid == 0) {
		dt <<= 1;
	}

	u64 event_time = nds->arm_cycles[cpuid] + dt;

	if (event_time < nds->arm_target_cycles[cpuid]) {
		nds->arm_target_cycles[cpuid] = event_time;
	}

	nds->scheduler.arm_events[cpuid][event].enabled = true;
	nds->scheduler.arm_events[cpuid][event].time = event_time;
}

void
run_events(NDS *nds)
{
	auto& sc = nds->scheduler;

	for (int i = 0; i < Scheduler::NUM_EVENTS; i++) {
		auto& event = sc.events[i];
		if (event.enabled && sc.current_time >= event.time) {
			event.enabled = false;
			if (event.cb) {
				event.cb(nds);
			}
		}
	}
}

void
run_arm_events(NDS *nds, int cpuid)
{
	auto& sc = nds->scheduler;

	for (int i = 0; i < Scheduler::NUM_ARM_EVENTS; i++) {
		auto& event = sc.arm_events[cpuid][i];
		if (event.enabled && nds->arm_cycles[cpuid] >= event.time) {
			event.enabled = false;
			if (event.cb) {
				event.cb(nds);
			}
		}
	}
}

} // namespace twice
