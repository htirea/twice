#include "nds/timer.h"

#include "nds/arm/arm.h"
#include "nds/nds.h"

namespace twice {

static int
get_freq_shift(u16 ctrl)
{
	u16 bits = ctrl & 3;
	if (bits == 0) {
		return 10;
	} else {
		return 10 - (4 + (bits << 1));
	}
}

static void
update_timer_counter(nds_ctx *nds, int cpuid, int timer_id)
{
	auto& t = nds->tmr[cpuid][timer_id];

	if (t.ctrl & BIT(7) && !(t.ctrl & BIT(2))) {
		timestamp current_time = nds->arm_cycles[cpuid];
		timestamp elapsed = current_time - t.last_update;
		if (cpuid == 0) {
			elapsed >>= 1;
		}

		t.counter += elapsed << t.shift;
		t.last_update = current_time;
	}
}

u16
read_timer_counter(nds_ctx *nds, int cpuid, int timer_id)
{
	auto& t = nds->tmr[cpuid][timer_id];
	update_timer_counter(nds, cpuid, timer_id);
	return t.counter >> 10;
}

static void
stop_timer(nds_ctx *nds, int cpuid, int timer_id)
{
	int event = event_scheduler::TIMER0_OVERFLOW + timer_id;
	cancel_cpu_event(nds, cpuid, event);
}

static void
schedule_timer_overflow(nds_ctx *nds, int cpuid, int timer_id)
{
	auto& t = nds->tmr[cpuid][timer_id];
	timestamp dt = ((u32)0x10000 << 10) - (t.counter & MASK(26));
	dt >>= t.shift;
	int event = event_scheduler::TIMER0_OVERFLOW + timer_id;
	schedule_cpu_event_after(nds, cpuid, event, dt);
}

void
write_timer_ctrl(nds_ctx *nds, int cpuid, int timer_id, u8 value)
{
	auto& t = nds->tmr[cpuid][timer_id];
	bool old_enabled = t.ctrl & BIT(7);
	bool new_enabled = value & BIT(7);

	if (!old_enabled && !new_enabled) {
		t.ctrl = value & 0xC7;
		t.shift = get_freq_shift(value);
		return;
	}

	if (old_enabled && !new_enabled) {
		update_timer_counter(nds, cpuid, timer_id);
		stop_timer(nds, cpuid, timer_id);
		t.ctrl = value & 0xC7;
		t.shift = get_freq_shift(value);
		return;
	}

	if (!old_enabled && new_enabled) {
		t.counter = (u32)t.reload_val << 10;
		t.last_update = nds->arm_cycles[cpuid];
	} else if (new_enabled) {
		update_timer_counter(nds, cpuid, timer_id);
		if (value & BIT(2)) {
			stop_timer(nds, cpuid, timer_id);
		}
	}

	t.ctrl = value & 0xC7;
	t.shift = get_freq_shift(value);

	if (!(value & BIT(2))) {
		schedule_timer_overflow(nds, cpuid, timer_id);
	}
}

static void
on_timer_overflow(nds_ctx *nds, int cpuid, int timer_id)
{
	auto& t = nds->tmr[cpuid][timer_id];

	if (!(t.ctrl & BIT(7))) {
		throw twice_error("timer should not be enabled");
	}

	timestamp current_time = nds->arm_cycles[cpuid];
	t.counter = (u32)t.reload_val << 10;
	t.last_update = current_time;

	if (t.ctrl & BIT(6)) {
		request_interrupt(nds->cpu[cpuid], 3 + timer_id);
	}

	if (timer_id != 3) {
		auto& next_t = nds->tmr[cpuid][timer_id + 1];
		if (next_t.ctrl & BIT(7) && next_t.ctrl & BIT(2)) {
			next_t.counter += 1 << 10;
			if ((next_t.counter >> 10 & 0xFFFF) == 0) {
				on_timer_overflow(nds, cpuid, timer_id + 1);
			}
		}
	}

	if (!(t.ctrl & BIT(2))) {
		schedule_timer_overflow(nds, cpuid, timer_id);
	}
}

void
event_timer_overflow(nds_ctx *nds, int cpuid, intptr_t timer_id)
{
	on_timer_overflow(nds, cpuid, timer_id);
}

} // namespace twice
