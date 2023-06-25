#ifndef TWICE_SCHEDULER_H
#define TWICE_SCHEDULER_H

#include "common/types.h"

namespace twice {

struct NDS;

typedef void (*event_cb)(NDS *);
typedef void (*arm_event_cb)(NDS *, int);

struct Scheduler {
	Scheduler();

	enum EventType {
		HBLANK_START,
		HBLANK_END,
		NUM_EVENTS,
	};

	enum ArmEventType {
		START_IMMEDIATE_DMAS,
		NUM_ARM_EVENTS,
	};

	struct Event {
		bool enabled{};
		u64 time{};
		event_cb cb{};
	};

	struct ArmEvent {
		bool enabled{};
		u64 time{};
		arm_event_cb cb{};
	};

	u64 current_time{};

	Event events[NUM_EVENTS];
	ArmEvent arm_events[2][NUM_ARM_EVENTS];
};

u64 get_next_event_time(NDS *nds);

void schedule_event(NDS *nds, int event, u64 t);
void reschedule_event_after(NDS *nds, int event, u64 dt);
void schedule_arm_event_after(NDS *nds, int cpuid, int event, u64 dt);

void run_events(NDS *nds);
void run_arm_events(NDS *nds, int cpuid);

} // namespace twice

#endif
