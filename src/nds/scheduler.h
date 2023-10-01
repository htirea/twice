#ifndef TWICE_SCHEDULER_H
#define TWICE_SCHEDULER_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

typedef void (*nds_event_cb)(nds_ctx *);
typedef void (*cpu_event_cb)(nds_ctx *nds, int cpuid, intptr_t data);

struct event_scheduler {
	event_scheduler();

	enum {
		HBLANK_START,
		HBLANK_END,
		ROM_ADVANCE_TRANSFER,
		AUXSPI_TRANSFER_COMPLETE,
		SAMPLE_AUDIO,
		NUM_NDS_EVENTS,
	};

	enum {
		START_IMMEDIATE_DMAS,
		TIMER0_OVERFLOW,
		TIMER1_OVERFLOW,
		TIMER2_OVERFLOW,
		TIMER3_OVERFLOW,
		SPI_TRANSFER_COMPLETE,
		NUM_CPU_EVENTS,
	};

	struct nds_event {
		bool enabled{};
		timestamp time{};
		nds_event_cb cb{};
	};

	struct arm_event {
		bool enabled{};
		timestamp time{};
		cpu_event_cb cb{};
		intptr_t data{};
	};

	timestamp current_time{};

	nds_event events[NUM_NDS_EVENTS];
	arm_event arm_events[2][NUM_CPU_EVENTS];
};

timestamp get_next_event_time(nds_ctx *nds);

void schedule_nds_event(nds_ctx *nds, int event, timestamp t);
void schedule_nds_event_after(
		nds_ctx *nds, int cpuid, int event, timestamp dt);
void reschedule_nds_event_after(nds_ctx *nds, int event, timestamp dt);
void schedule_cpu_event_after(
		nds_ctx *nds, int cpuid, int event, timestamp dt);
void cancel_cpu_event(nds_ctx *nds, int cpuid, int event);

void run_nds_events(nds_ctx *nds);
void run_cpu_events(nds_ctx *nds, int cpuid);

} // namespace twice

#endif
