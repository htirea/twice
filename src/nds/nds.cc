#include "nds/nds.h"
#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/mem/bus.h"
#include "nds/mem/io.h"

#include "common/util.h"

#include "libtwice/exception.h"

namespace twice {

nds_ctx::nds_ctx(u8 *arm7_bios, u8 *arm9_bios, u8 *firmware, u8 *cartridge,
		size_t cartridge_size)
	: arm9(std::make_unique<arm9_cpu>(this)),
	  arm7(std::make_unique<arm7_cpu>(this)),
	  gpu2d{ { this, 0 }, { this, 1 } },
	  dma{ { this, 0 }, { this, 1 } },
	  arm7_bios(arm7_bios),
	  arm9_bios(arm9_bios),
	  firmware(firmware),
	  cartridge(cartridge),
	  cartridge_size(cartridge_size)
{
	cpu[0] = arm9.get();
	cpu[1] = arm7.get();

	/* need these for side effects */
	wramcnt_write(this, 0x0);
	powcnt1_write(this, 0x0);

	schedule_nds_event(this, event_scheduler::HBLANK_START, 1536);
	schedule_nds_event(this, event_scheduler::HBLANK_END, 2130);
}

nds_ctx::~nds_ctx() = default;

static void
parse_header(nds_ctx *nds, u32 *entry_addr_ret)
{
	u32 rom_offset[2]{ readarr<u32>(nds->cartridge, 0x20),
		readarr<u32>(nds->cartridge, 0x30) };
	u32 entry_addr[2]{ readarr<u32>(nds->cartridge, 0x24),
		readarr<u32>(nds->cartridge, 0x34) };
	u32 ram_addr[2]{ readarr<u32>(nds->cartridge, 0x28),
		readarr<u32>(nds->cartridge, 0x38) };
	u32 transfer_size[2]{ readarr<u32>(nds->cartridge, 0x2C),
		readarr<u32>(nds->cartridge, 0x3C) };

	if (rom_offset[0] >= nds->cartridge_size) {
		throw twice_error("arm9 rom offset too large");
	}

	if (rom_offset[0] + transfer_size[0] > nds->cartridge_size) {
		throw twice_error("arm9 transfer size too large");
	}

	if (rom_offset[1] >= nds->cartridge_size) {
		throw twice_error("arm7 rom offset too large");
	}

	if (rom_offset[1] + transfer_size[1] > nds->cartridge_size) {
		throw twice_error("arm7 transfer size too large");
	}

	for (u32 i = 0; i < transfer_size[0]; i++) {
		u8 value = readarr<u8>(nds->cartridge, rom_offset[0] + i);
		bus9_write<u8>(nds, ram_addr[0] + i, value);
	}

	for (u32 i = 0; i < transfer_size[1]; i++) {
		u8 value = readarr<u8>(nds->cartridge, rom_offset[1] + i);
		bus7_write<u8>(nds, ram_addr[1] + i, value);
	}

	entry_addr_ret[0] = entry_addr[0] & ~3;
	entry_addr_ret[1] = entry_addr[1] & ~3;
}

void
nds_direct_boot(nds_ctx *nds)
{
	/* give shared wram to arm7 before we do anything else */
	wramcnt_write(nds, 0x3);
	powcnt1_write(nds, 0x1);
	nds->soundbias = 0x200;
	nds->postflg[0] = 0x1;
	nds->postflg[1] = 0x1;

	nds->arm9->cp15_write(0x100, 0x00012078);
	nds->arm9->cp15_write(0x910, 0x0300000A);
	nds->arm9->cp15_write(0x911, 0x00000020);

	u32 entry_addr[2];
	parse_header(nds, entry_addr);

	nds->arm9->gpr[12] = entry_addr[0];
	nds->arm9->gpr[13] = 0x03002F7C;
	nds->arm9->gpr[14] = entry_addr[0];
	nds->arm9->bankedr[MODE_IRQ][0] = 0x03003F80;
	nds->arm9->bankedr[MODE_SVC][0] = 0x03003FC0;
	nds->arm9->jump(entry_addr[0]);

	nds->arm7->gpr[12] = entry_addr[1];
	nds->arm7->gpr[13] = 0x0380FD80;
	nds->arm7->gpr[14] = entry_addr[1];
	nds->arm7->bankedr[MODE_IRQ][0] = 0x0380FF80;
	nds->arm7->bankedr[MODE_SVC][0] = 0x0380FFC0;
	nds->arm7->jump(entry_addr[1]);

	/* TODO: more stuff for direct booting */
}

void
nds_run_frame(nds_ctx *nds)
{
	nds->frame_finished = false;

	while (!nds->frame_finished) {
		nds->arm_target_cycles[0] = get_next_event_time(nds);
		if (nds->dma[0].active) {
			run_dma9(nds);
		} else {
			run_cpu(nds->cpu[0]);
		}

		run_cpu_events(nds, 0);

		u64 arm7_target = nds->arm_cycles[0] >> 1;
		while (nds->arm_cycles[1] < arm7_target) {
			nds->arm_target_cycles[1] = arm7_target;
			if (nds->dma[1].active) {
				run_dma7(nds);
			} else {
				run_cpu(nds->cpu[1]);
			}

			run_cpu_events(nds, 1);
			nds->arm7->check_halted();

			if (nds->arm7->interrupt) {
				do_irq(nds->cpu[1]);
			}
		}

		nds->scheduler.current_time = nds->arm_cycles[0];
		run_nds_events(nds);

		nds->arm9->check_halted();
		nds->arm7->check_halted();

		if (nds->arm9->interrupt) {
			do_irq(nds->cpu[0]);
		}

		if (nds->arm7->interrupt) {
			do_irq(nds->cpu[1]);
		}
	}
}

void
event_hblank_start(nds_ctx *nds)
{
	nds->dispstat[0] |= BIT(1);
	nds->dispstat[1] |= BIT(1);

	if (nds->dispstat[0] & BIT(4)) {
		request_interrupt(nds->cpu[0], 1);
	}

	if (nds->dispstat[1] & BIT(4)) {
		request_interrupt(nds->cpu[1], 1);
	}

	dma_on_hblank_start(nds);

	reschedule_nds_event_after(nds, event_scheduler::HBLANK_START, 2130);
}

static u16
get_lyc(u16 dispstat)
{
	return (dispstat & BIT(7)) << 1 | dispstat >> 8;
}

static void
nds_on_vblank(nds_ctx *nds)
{
	nds->dispstat[0] |= BIT(0);
	nds->dispstat[1] |= BIT(0);

	if (nds->dispstat[0] & BIT(3)) {
		request_interrupt(nds->cpu[0], 0);
	}

	if (nds->dispstat[1] & BIT(3)) {
		request_interrupt(nds->cpu[1], 0);
	}

	dma_on_vblank(nds);

	nds->frame_finished = true;
}

void
event_hblank_end(nds_ctx *nds)
{
	nds->vcount += 1;
	if (nds->vcount == 263) {
		nds->vcount = 0;
	}

	nds->dispstat[0] &= ~BIT(1);
	nds->dispstat[1] &= ~BIT(1);

	/* the next scanline starts here */

	for (int i = 0; i < 2; i++) {
		u16 lyc = get_lyc(nds->dispstat[i]);
		if (nds->vcount == lyc) {
			nds->dispstat[i] |= BIT(2);
			if (nds->dispstat[i] & BIT(5)) {
				request_interrupt(nds->cpu[i], 2);
			}
		} else {
			nds->dispstat[i] &= ~BIT(2);
		}
	}

	if (nds->vcount == 192) {
		nds_on_vblank(nds);
	} else if (nds->vcount == 262) {
		/* vblank flag isn't set in last scanline */
		nds->dispstat[0] &= ~BIT(0);
		nds->dispstat[1] &= ~BIT(0);
	}

	gpu_on_scanline_start(nds);

	reschedule_nds_event_after(nds, event_scheduler::HBLANK_END, 2130);
}

} // namespace twice
