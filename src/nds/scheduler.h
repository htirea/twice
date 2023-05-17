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

	u64 current_time{};

	struct Event {
		bool enabled{};
		u64 time{};
		event_cb cb{};
	} events[NUM_EVENTS];

	void schedule_event(int event, u64 t);
	void reschedule_event_after(int event, u64 t);
	u64 get_next_event_time();
};

void run_events(NDS *nds);

} // namespace twice

#endif
