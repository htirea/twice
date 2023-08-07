#ifndef TWICE_TIMER_H
#define TWICE_TIMER_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct timer {
	u32 counter{};
	u16 reload_val{};
	u16 ctrl{};
	timestamp last_update{};
	int shift{};
};

u16 read_timer_counter(nds_ctx *nds, int cpuid, int timer_id);
void write_timer_ctrl(nds_ctx *nds, int cpuid, int timer_id, u8 value);

void event_timer_overflow(nds_ctx *nds, int cpuid, intptr_t timer_id);

} // namespace twice

#endif
