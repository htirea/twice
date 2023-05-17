#ifndef TWICE_SCHEDULER_H
#define TWICE_SCHEDULER_H

#include "common/types.h"

namespace twice {

struct NDS;

typedef void (*event_cb)(NDS *);

struct Scheduler {
	Scheduler();

	enum EventType {
		HBLANK_START,
		HBLANK_END,
		NUM_EVENTS,
	};

	enum ArmEventType {
		NULL_EVENT,
		NUM_ARM_EVENTS,
	};

	struct Event {
		bool enabled{};
		u64 time{};
		event_cb cb{};
	};

	u64 current_time{};

	Event events[NUM_EVENTS];
	Event arm_events[2][NUM_ARM_EVENTS];

	void schedule_event(int event, u64 t);
	void reschedule_event_after(int event, u64 dt);
	u64 get_next_event_time();
};

void schedule_arm_event_after(NDS *nds, int cpuid, int event, u64 dt);
void run_events(NDS *nds);
void run_arm_events(NDS *nds, int cpuid);

} // namespace twice

#endif
