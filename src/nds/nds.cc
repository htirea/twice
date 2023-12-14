#include "nds/nds.h"
#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/cart/key.h"
#include "nds/mem/bus.h"
#include "nds/mem/io.h"

#include "common/util.h"

#include "libtwice/exception.h"

namespace twice {

nds_ctx::nds_ctx(u8 *arm7_bios, u8 *arm9_bios, u8 *firmware, u8 *cartridge,
		size_t cartridge_size, u8 *savefile, size_t savefile_size,
		nds_savetype savetype)
	: arm9(std::make_unique<arm9_cpu>(this)),
	  arm7(std::make_unique<arm7_cpu>(this)),
	  gpu2d{ { this, 0 }, { this, 1 } },
	  gpu3d{ this },
	  dma{ { this, 0 }, { this, 1 } },
	  fw(firmware),
	  cart(cartridge, cartridge_size, savefile, savefile_size, savetype,
			  arm7_bios),
	  arm7_bios(arm7_bios),
	  arm9_bios(arm9_bios)
{
	cpu[0] = arm9.get();
	cpu[1] = arm7.get();

	/* need these for side effects */
	wramcnt_write(this, 0x0);
	powcnt1_write(this, 0x0);

	schedule_nds_event(this, event_scheduler::HBLANK_START, 1536);
	schedule_nds_event(this, event_scheduler::HBLANK_END, 2130);
	/* TODO: sample rate is not exact */
	schedule_nds_event(this, event_scheduler::SAMPLE_AUDIO, 1024);
}

nds_ctx::~nds_ctx() = default;

void
nds_firmware_boot(nds_ctx *nds)
{
	nds->soundbias = 0x200;
	encrypt_secure_area(&nds->cart);
	update_arm9_page_tables(nds->arm9.get());
	update_arm7_page_tables(nds->arm7.get());
	nds->arm9->arm_jump(0xFFFF0000);
	nds->arm7->arm_jump(0x0);
}

static void parse_header(nds_ctx *nds, u32 *entry_addr_ret);

void
nds_direct_boot(nds_ctx *nds)
{
	/* give shared wram to arm7 before we do anything else */
	wramcnt_write(nds, 0x3);
	powcnt1_write(nds, 0x1);
	nds->soundbias = 0x200;
	nds->postflg[0] = 0x1;
	nds->postflg[1] = 0x1;

	u32 chip_id = nds->cart.chip_id;
	writearr<u32>(nds->main_ram, 0x3FF800, chip_id);
	writearr<u32>(nds->main_ram, 0x3FF804, chip_id);
	writearr<u16>(nds->main_ram, 0x3FF850, 0x5835);
	writearr<u32>(nds->main_ram, 0x3FF880, 0x7);
	writearr<u32>(nds->main_ram, 0x3FF884, 0x6);
	writearr<u32>(nds->main_ram, 0x3FFC00, chip_id);
	writearr<u32>(nds->main_ram, 0x3FFC04, chip_id);
	writearr<u16>(nds->main_ram, 0x3FFC10, 0x5835);
	writearr<u16>(nds->main_ram, 0x3FFC40, 0x1);

	std::memcpy(nds->main_ram + 0x3FFC80, nds->fw.user_settings, 0x70);

	u32 entry_addr[2];
	parse_header(nds, entry_addr);
	std::memcpy(nds->main_ram + 0x3FFE00, nds->cart.data,
			std::min((size_t)0x170, nds->cart.size));

	arm9_direct_boot(nds->arm9.get(), entry_addr[0]);
	arm7_direct_boot(nds->arm7.get(), entry_addr[1]);

	/* TODO: more stuff for direct booting */
}

static void
parse_header(nds_ctx *nds, u32 *entry_addr_ret)
{
	auto& cart = nds->cart;

	u32 rom_offset[2]{ readarr<u32>(cart.data, 0x20),
		readarr<u32>(cart.data, 0x30) };
	u32 entry_addr[2]{ readarr<u32>(cart.data, 0x24),
		readarr<u32>(cart.data, 0x34) };
	u32 ram_addr[2]{ readarr<u32>(cart.data, 0x28),
		readarr<u32>(cart.data, 0x38) };
	u32 transfer_size[2]{ readarr<u32>(cart.data, 0x2C),
		readarr<u32>(cart.data, 0x3C) };

	if (rom_offset[0] >= cart.size) {
		throw twice_error("arm9 rom offset too large");
	}

	if (rom_offset[0] + transfer_size[0] > cart.size) {
		throw twice_error("arm9 transfer size too large");
	}

	if (rom_offset[1] >= cart.size) {
		throw twice_error("arm7 rom offset too large");
	}

	if (rom_offset[1] + transfer_size[1] > cart.size) {
		throw twice_error("arm7 transfer size too large");
	}

	for (u32 i = 0; i < transfer_size[0]; i++) {
		u8 value = readarr<u8>(cart.data, rom_offset[0] + i);
		bus9_write<u8>(nds, ram_addr[0] + i, value);
	}

	for (u32 i = 0; i < transfer_size[1]; i++) {
		u8 value = readarr<u8>(cart.data, rom_offset[1] + i);
		bus7_write<u8>(nds, ram_addr[1] + i, value);
	}

	entry_addr_ret[0] = entry_addr[0] & ~3;
	entry_addr_ret[1] = entry_addr[1] & ~3;
}

void
nds_run_frame(nds_ctx *nds)
{
	nds->frame_finished = false;

	arm9_frame_start(nds->arm9.get());
	arm7_frame_start(nds->arm7.get());
	sound_frame_start(nds);

	while (!nds->frame_finished) {
		nds->arm_target_cycles[0] = get_next_event_time(nds);
		if (nds->dma[0].active) {
			run_dma9(nds);
		} else {
			nds->arm9->run();
		}

		run_cpu_events(nds, 0);

		/* TODO: handle case when arm9 overflows */
		timestamp arm7_target = nds->arm_cycles[0] >> 1;
		while (nds->arm_cycles[1] < arm7_target) {
			nds->arm_target_cycles[1] = arm7_target;
			if (nds->dma[1].active) {
				run_dma7(nds);
			} else {
				nds->arm7->run();
			}

			run_cpu_events(nds, 1);
			nds->arm7->check_halted();

			if (nds->arm7->interrupt) {
				arm_do_irq(nds->cpu[1]);
			}
		}

		nds->scheduler.current_time = nds->arm_cycles[0];
		run_nds_events(nds);

		nds->arm9->check_halted();
		nds->arm7->check_halted();

		if (nds->arm9->interrupt) {
			arm_do_irq(nds->cpu[0]);
		}

		if (nds->arm7->interrupt) {
			arm_do_irq(nds->cpu[1]);
		}
	}

	arm9_frame_end(nds->arm9.get());
	arm7_frame_end(nds->arm7.get());
	sound_frame_end(nds);
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

static void nds_on_vblank(nds_ctx *nds);

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
		u16 lyc = (nds->dispstat[i] & 0x80) << 1 |
		          nds->dispstat[i] >> 8;
		if (nds->vcount == lyc) {
			nds->dispstat[i] |= BIT(2);
			if (nds->dispstat[i] & BIT(5)) {
				request_interrupt(nds->cpu[i], 2);
			}
		} else {
			nds->dispstat[i] &= ~BIT(2);
		}
	}

	if (nds->vcount == 262) {
		/* vblank flag isn't set in last scanline */
		nds->dispstat[0] &= ~BIT(0);
		nds->dispstat[1] &= ~BIT(0);
	}

	gpu3d_on_scanline_start(nds);
	gpu_on_scanline_start(nds);
	dma_on_scanline_start(nds);

	if (nds->vcount == 192) {
		nds_on_vblank(nds);
	}

	reschedule_nds_event_after(nds, event_scheduler::HBLANK_END, 2130);
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

	gpu3d_on_vblank(&nds->gpu3d);
	dma_on_vblank(nds);

	nds->frame_finished = true;
}

void
nds_dump_prof(nds_ctx *nds)
{
	nds->prof.report();
	LOG("arm9: fetch: %f, load %f, store %f\n",
			(double)nds->arm9->fetch_hits / nds->arm9->fetch_total,
			(double)nds->arm9->load_hits / nds->arm9->load_total,
			(double)nds->arm9->store_hits /
					nds->arm9->store_total);
	LOG("arm7: fetch: %f, load %f, store %f\n",
			(double)nds->arm7->fetch_hits / nds->arm7->fetch_total,
			(double)nds->arm7->load_hits / nds->arm7->load_total,
			(double)nds->arm7->store_hits /
					nds->arm7->store_total);
}

} // namespace twice
