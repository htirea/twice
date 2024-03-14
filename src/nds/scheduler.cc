#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/spi.h"

#include "libtwice/exception.h"

namespace twice {

struct event_state {
	scheduler::event_cb cb{};
	intptr_t data{};
	timestamp expiry{};
	bool enabled{};
};

static void run_events(
		nds_ctx *nds, u32 start, u32 length, timestamp curr_time);

static event_state initial_state[scheduler::NUM_EVENTS] = {
	{ .cb = event_hblank_start },
	{ .cb = event_hblank_end },
	{ .cb = event_32khz_tick },
	{ .cb = event_advance_rom_transfer },
	{ .cb = event_auxspi_transfer_complete },
	{ .cb = event_execution_target_reached },

	{ .cb = event_timer_update, .data = 0 },
	{ .cb = event_timer_update, .data = 1 },
	{ .cb = event_timer_update, .data = 2 },
	{ .cb = event_timer_update, .data = 3 },
	{ .cb = event_start_immediate_dmas, .data = 0 },

	{ .cb = event_timer_update, .data = 4 },
	{ .cb = event_timer_update, .data = 5 },
	{ .cb = event_timer_update, .data = 6 },
	{ .cb = event_timer_update, .data = 7 },
	{ .cb = event_start_immediate_dmas, .data = 1 },
	{ .cb = event_spi_transfer_complete },
};

void
scheduler_init(nds_ctx *nds)
{
	auto& sc = nds->sc;

	for (u32 i = 0; i < scheduler::NUM_EVENTS; i++) {
		sc.callbacks[i] = initial_state[i].cb;
		sc.data[i] = initial_state[i].data;
		sc.expiry[i] = initial_state[i].expiry;
		sc.enabled[i] = initial_state[i].enabled;
	}
}

timestamp
get_next_event_time(nds_ctx *nds)
{
	auto& sc = nds->sc;

	timestamp curr = nds->arm_cycles[0];
	timestamp limit = curr + 64;
	timestamp target = -1;

	for (int i = 0; i < scheduler::NUM_EVENTS; i++) {
		if (sc.enabled[i]) {
			target = std::min(target, sc.expiry[i]);
		}
	}

	bool fast_skip = !nds->dma[0].active && !nds->dma[1].active &&
	                 nds->cpu[0]->halted && nds->cpu[1]->halted;
	if (!fast_skip) {
		target = std::min(target, limit);
	}

	if (target <= curr) {
		for (int i = 0; i < scheduler::NUM_EVENTS; i++) {
			if (sc.enabled[i] && sc.expiry[i] == target) {
				LOG("%d\n", i);
			}
		}
		throw twice_error("target timestamp <= curr");
	}

	return target;
}

void
schedule_event(nds_ctx *nds, int id, timestamp t)
{
	auto& sc = nds->sc;
	sc.expiry[id] = t;
	sc.enabled[id] = true;
}

void
reschedule_event_after(nds_ctx *nds, int id, timestamp dt)
{
	auto& sc = nds->sc;
	sc.expiry[id] += dt;
	sc.enabled[id] = true;
}

void
schedule_event_after(nds_ctx *nds, int cpuid, int id, timestamp dt)
{
	auto& sc = nds->sc;
	timestamp t = (nds->arm_cycles[cpuid] << cpuid) + dt;

	if (t < nds->arm_target_cycles[cpuid] << cpuid) {
		nds->arm_target_cycles[cpuid] = t >> cpuid;
	}

	sc.enabled[id] = true;
	sc.expiry[id] = t;
}

void
cancel_event(nds_ctx *nds, int id)
{
	auto& sc = nds->sc;
	sc.enabled[id] = false;
}

void
run_system_events(nds_ctx *nds)
{
	run_events(nds, scheduler::SYSTEM_EVENTS, scheduler::ARM9_EVENTS,
			nds->arm_cycles[0]);
	nds->arm9->check_halted();
	nds->arm7->check_halted();
}

void
run_cpu_events(nds_ctx *nds, int cpuid)
{
	if (cpuid == 0) {
		run_events(nds, scheduler::ARM9_EVENTS, scheduler::ARM7_EVENTS,
				nds->arm_cycles[0]);
	} else {
		run_events(nds, scheduler::ARM7_EVENTS, scheduler::NUM_EVENTS,
				nds->arm_cycles[1] << 1);
		nds->arm7->check_halted();
	}
}

int
get_timer_update_event_id(int cpuid, int timer_id)
{
	int id = cpuid == 0 ? scheduler::ARM9_TIMER0_UPDATE
	                    : scheduler::ARM7_TIMER0_UPDATE;
	return id + timer_id;
}

static void
run_events(nds_ctx *nds, u32 start, u32 end, timestamp curr_time)
{
	auto& sc = nds->sc;

	for (u32 i = start; i < end; i++) {
		if (!sc.enabled[i])
			continue;

		if (curr_time >= sc.expiry[i]) {
			timestamp late = curr_time - sc.expiry[i];
			sc.enabled[i] = false;
			sc.callbacks[i](nds, sc.data[i], late);
		}
	}
}

} // namespace twice
