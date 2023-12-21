#ifndef TWICE_SCHEDULER_H
#define TWICE_SCHEDULER_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct scheduler {
	typedef void (*event_cb)(nds_ctx *nds, intptr_t data, timestamp late);

	enum {
		/* both cpus */
		HBLANK_START,
		HBLANK_END,
		SAMPLE_AUDIO,
		RTC_TICK,
		CART_TRANSFER,
		AUXSPI_TRANSFER,

		/* arm9 */
		ARM9_TIMER0_OVERFLOW,
		ARM9_TIMER1_OVERFLOW,
		ARM9_TIMER2_OVERFLOW,
		ARM9_TIMER3_OVERFLOW,
		ARM9_IMMEDIATE_DMA,

		/* arm 7 */
		ARM7_TIMER0_OVERFLOW,
		ARM7_TIMER1_OVERFLOW,
		ARM7_TIMER2_OVERFLOW,
		ARM7_TIMER3_OVERFLOW,
		ARM7_IMMEDIATE_DMA,
		ARM7_SPI_TRANSFER,

		NUM_EVENTS,
	};

	event_cb callbacks[NUM_EVENTS]{};
	intptr_t data[NUM_EVENTS]{};
	timestamp expiry[NUM_EVENTS]{};
	bool enabled[NUM_EVENTS]{};
};

void scheduler_init(nds_ctx *nds);
timestamp get_next_event_time(nds_ctx *nds);
void schedule_event_after(nds_ctx *nds, int id, timestamp dt);
void schedule_event_after(nds_ctx *nds, int cpuid, int id, timestamp dt);
void cancel_event(nds_ctx *nds, int id);
void run_cpu_events(nds_ctx *nds, int cpuid);
void run_system_events(nds_ctx *nds);
int get_timer_overflow_event_id(int cpuid, int timer_id);

} // namespace twice

#endif
