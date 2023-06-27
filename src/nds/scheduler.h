#ifndef TWICE_SCHEDULER_H
#define TWICE_SCHEDULER_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

typedef void (*nds_event_cb)(nds_ctx *);
typedef void (*cpu_event_cb)(nds_ctx *, int);

struct event_scheduler {
	event_scheduler();

	enum {
		HBLANK_START,
		HBLANK_END,
		NUM_NDS_EVENTS,
	};

	enum {
		START_IMMEDIATE_DMAS,
		NUM_CPU_EVENTS,
	};

	struct nds_event {
		bool enabled{};
		u64 time{};
		nds_event_cb cb{};
	};

	struct arm_event {
		bool enabled{};
		u64 time{};
		cpu_event_cb cb{};
	};

	u64 current_time{};

	nds_event events[NUM_NDS_EVENTS];
	arm_event arm_events[2][NUM_CPU_EVENTS];
};

u64 get_next_event_time(nds_ctx *nds);

void schedule_nds_event(nds_ctx *nds, int event, u64 t);
void reschedule_nds_event_after(nds_ctx *nds, int event, u64 dt);
void schedule_cpu_event_after(nds_ctx *nds, int cpuid, int event, u64 dt);

void run_nds_events(nds_ctx *nds);
void run_cpu_events(nds_ctx *nds, int cpuid);

} // namespace twice

#endif
