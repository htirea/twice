#include "nds/nds.h"
#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/mem/bus.h"
#include "nds/mem/io.h"

#include "common/util.h"

#include "libtwice/exception.h"

namespace twice {

NDS::NDS(u8 *arm7_bios, u8 *arm9_bios, u8 *firmware, u8 *cartridge,
		size_t cartridge_size)
	: arm9(std::make_unique<Arm9>(this)),
	  arm7(std::make_unique<Arm7>(this)),
	  gpu2D{ { this, 0 }, { this, 1 } },
	  dma9(this),
	  dma7(this),
	  arm7_bios(arm7_bios),
	  arm9_bios(arm9_bios),
	  firmware(firmware),
	  cartridge(cartridge),
	  cartridge_size(cartridge_size)
{
	cpu[0] = arm9.get();
	cpu[1] = arm7.get();

	wramcnt_write(this, 0x3);
	powcnt1_write(this, 0x0);

	scheduler.schedule_event(Scheduler::HBLANK_START, 1536);
	scheduler.schedule_event(Scheduler::HBLANK_END, 2130);
}

NDS::~NDS() = default;

static void
parse_header(NDS *nds, u32 *entry_addr_ret)
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
		throw TwiceError("arm9 rom offset too large");
	}

	if (rom_offset[0] + transfer_size[0] > nds->cartridge_size) {
		throw TwiceError("arm9 transfer size too large");
	}

	if (rom_offset[1] >= nds->cartridge_size) {
		throw TwiceError("arm7 rom offset too large");
	}

	if (rom_offset[1] + transfer_size[1] > nds->cartridge_size) {
		throw TwiceError("arm7 transfer size too large");
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
NDS::direct_boot()
{
	u32 entry_addr[2];

	parse_header(this, entry_addr);

	arm9->cp15_write(0x100, 0x00012078);
	arm9->cp15_write(0x910, 0x0300000A);
	arm9->cp15_write(0x911, 0x00000020);

	/* TODO: remove this later */
	arm9->cp15_write(0x910, 0x0080000A);

	arm9->gpr[12] = entry_addr[0];
	arm9->gpr[13] = 0x03002F7C;
	arm9->gpr[14] = entry_addr[0];
	arm9->bankedr[MODE_IRQ][0] = 0x03003F80;
	arm9->bankedr[MODE_SVC][0] = 0x03003FC0;
	arm9->jump(entry_addr[0]);

	arm7->gpr[12] = entry_addr[1];
	arm7->gpr[13] = 0x0380FD80;
	arm7->gpr[14] = entry_addr[1];
	arm7->bankedr[MODE_IRQ][0] = 0x0380FF80;
	arm7->bankedr[MODE_SVC][0] = 0x0380FFC0;
	arm7->jump(entry_addr[1]);

	/* TODO: more stuff for direct booting */
}

void
NDS::run_frame()
{
	frame_finished = false;

	while (!frame_finished) {
		u64 next_event = scheduler.get_next_event_time();

		arm_target_cycles[0] = next_event << 1;
		if (dma9.active) {
			dma9.run();
		} else {
			arm9->run();
		}

		run_arm_events(this, 0);

		u64 arm7_target = arm_cycles[0] >> 1;
		while (arm_cycles[1] < arm7_target) {
			arm_target_cycles[1] = arm7_target;
			if (dma7.active) {
				dma7.run();
			} else {
				arm7->run();
			}

			run_arm_events(this, 1);
			arm7->check_halted();
		}

		scheduler.current_time = arm_cycles[1];
		run_events(this);

		arm9->check_halted();
		arm7->check_halted();
	}
}

void
nds_on_vblank(NDS *nds)
{
	nds->dispstat[0] |= BIT(0);
	nds->dispstat[1] |= BIT(0);

	if (nds->dispstat[0] & BIT(3)) {
		nds->cpu[0]->request_interrupt(0);
	}

	if (nds->dispstat[1] & BIT(3)) {
		nds->cpu[1]->request_interrupt(0);
	}

	dma_on_vblank(nds);

	nds->frame_finished = true;
}

void
nds_event_hblank_start(NDS *nds)
{
	nds->dispstat[0] |= BIT(1);
	nds->dispstat[1] |= BIT(1);

	if (nds->dispstat[0] & BIT(4)) {
		nds->cpu[0]->request_interrupt(1);
	}

	if (nds->dispstat[1] & BIT(4)) {
		nds->cpu[1]->request_interrupt(1);
	}

	dma_on_hblank_start(nds);

	nds->scheduler.reschedule_event_after(Scheduler::HBLANK_START, 2130);
}

static u16
get_lyc(u16 dispstat)
{
	return (dispstat & BIT(7)) << 1 | dispstat >> 8;
}

void
nds_event_hblank_end(NDS *nds)
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
				nds->cpu[i]->request_interrupt(2);
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

	nds->scheduler.reschedule_event_after(Scheduler::HBLANK_END, 2130);
}

} // namespace twice
