#include "nds/nds.h"
#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/cart/key.h"
#include "nds/mem/io.h"

#include "common/util.h"

#include "libtwice/exception.h"

namespace twice {

using namespace twice::fs;

static void nds_setup_run(nds_ctx *nds, u64 target, unsigned long term_sigs,
		s16 *mic_buf, size_t mic_buf_len, nds_exec *out);
static void run_loop(nds_ctx *nds);
static void check_lyc(nds_ctx *nds, int cpuid);
static void nds_on_vblank(nds_ctx *nds);
static void schedule_32k_tick_event(nds_ctx *nds, timestamp late);

nds_ctx::~nds_ctx() = default;

std::unique_ptr<nds_ctx>
create_nds_ctx(file_view arm9_bios, file_view arm7_bios, file_view firmware,
		file_view cart, file save, nds_savetype savetype, file image,
		nds_config *config)
{
	auto ctx = std::make_unique<nds_ctx>();
	nds_ctx *nds = ctx.get();
	nds->config = config;
	nds->arm9_bios_v = std::move(arm9_bios);
	nds->arm7_bios_v = std::move(arm7_bios);
	nds->firmware_v = std::move(firmware);
	nds->cart_v = std::move(cart);
	nds->save_v = save.cmap();
	nds->savefile = std::move(save);
	nds->image_v = image.cmap();
	nds->image = std::move(image);

	nds->arm9_bios = nds->arm9_bios_v.data();
	nds->arm7_bios = nds->arm7_bios_v.data();
	nds->arm9 = std::make_unique<arm9_cpu>();
	nds->arm7 = std::make_unique<arm7_cpu>();
	nds->cpu[0] = nds->arm9.get();
	nds->cpu[1] = nds->arm7.get();
	arm_init(nds, 0);
	arm_init(nds, 1);
	gpu2d_init(nds);
	gpu3d_init(nds);
	firmware_init(nds);
	cartridge_init(nds, savetype);
	dma_controller_init(nds, 0);
	dma_controller_init(nds, 1);
	scheduler_init(nds);
	page_tables_init(nds);

	/* need these for side effects */
	wramcnt_write(nds, 0x0);
	powcnt1_write(nds, 0x0);
	schedule_event_after(nds, scheduler::HBLANK_START, 3072);
	schedule_event_after(nds, scheduler::HBLANK_END, 4260);
	schedule_32k_tick_event(nds, 0);

	return ctx;
}

void
nds_firmware_boot(nds_ctx *nds)
{
	nds->soundbias = 0x200;
	encrypt_secure_area(&nds->cart);
	nds->arm9->arm_jump(0xFFFF0000);
	nds->arm7->arm_jump(0x0);
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
	/* TODO: call write handler instead */
	nds->pwr.ctrl = 0xD;

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
	parse_cart_header(nds, entry_addr);
	std::memcpy(nds->main_ram + 0x3FFE00, nds->cart.data,
			std::min((size_t)0x170, nds->cart.size));

	arm9_direct_boot(nds->arm9.get(), entry_addr[0]);
	arm7_direct_boot(nds->arm7.get(), entry_addr[1]);

	/* TODO: more stuff for direct booting */
}

void
nds_run(nds_ctx *nds, run_mode mode, const nds_exec *in, nds_exec *out)
{
	u64 target = -1;
	unsigned long term_sigs = 0;
	s16 *mic_buf = nullptr;
	size_t mic_buf_len = 0;

	if (in) {
		switch (mode) {
		case run_mode::RUN_FOR:
			target = nds->arm_cycles[0] + in->cycles;
			break;
		case run_mode::RUN_UNTIL:
			target = in->cycles;
			break;
		default:
			target = -1;
		}

		term_sigs = in->sig_flags;
		mic_buf = in->audio_buf;
		mic_buf_len = in->audio_buf_len;
	}

	if (mode == run_mode::RUN_UNTIL_VBLANK) {
		term_sigs |= nds_signal::VBLANK;
	}

	nds_setup_run(nds, target, term_sigs, mic_buf, mic_buf_len, out);
	run_loop(nds);
}

void
nds_sync_files(nds_ctx *nds, bool sync_whole_file)
{
	sync_savefile(nds, sync_whole_file);
	sync_image_file(nds, sync_whole_file);
}

void
nds_dump_prof(nds_ctx *nds)
{
	nds->prof.report();
}

void
event_execution_target_reached(nds_ctx *nds, intptr_t, timestamp late)
{
	nds->execution_finished = true;
}

void
event_hblank_start(nds_ctx *nds, intptr_t, timestamp late)
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

	schedule_event_after(nds, scheduler::HBLANK_START, 4260 - late);
}

void
event_hblank_end(nds_ctx *nds, intptr_t, timestamp late)
{
	nds->vcount += 1;
	if (nds->vcount == 263) {
		nds->vcount = 0;
	}

	nds->dispstat[0] &= ~BIT(1);
	nds->dispstat[1] &= ~BIT(1);
	check_lyc(nds, 0);
	check_lyc(nds, 1);
	gpu3d_on_scanline_start(nds);
	gpu_on_scanline_start(nds);
	dma_on_scanline_start(nds);

	if (nds->vcount == 192) {
		nds_on_vblank(nds);
	}

	if (nds->vcount == 262) {
		nds->dispstat[0] &= ~BIT(0);
		nds->dispstat[1] &= ~BIT(0);
	}

	schedule_event_after(nds, scheduler::HBLANK_END, 4260 - late);
}

void
event_32khz_tick(nds_ctx *nds, intptr_t data, timestamp late)
{
	nds->timer_32k_ticks++;
	touchscreen_tick_32k(nds);
	rtc_tick_32k(nds, late);
	event_sample_audio(nds, data, late);
	nds->mic_buf_idx++;
	schedule_32k_tick_event(nds, late);
}

static void
nds_setup_run(nds_ctx *nds, u64 target, unsigned long term_sigs, s16 *mic_buf,
		size_t mic_buf_len, nds_exec *out)
{
	nds->audio_buf.fill(0);
	nds->audio_buf_idx = 0;
	nds->mic_buf.fill(0);
	nds->mic_buf_idx = 0;

	if (mic_buf) {
		size_t count = std::min(nds->mic_buf.size(), mic_buf_len);
		std::copy(mic_buf, mic_buf + count, nds->mic_buf.begin());
	}

	nds->term_sigs = term_sigs;
	nds->raised_sigs = 0;
	nds->arm9->cycles_executed = 0;
	nds->arm7->cycles_executed = 0;
	nds->dma[0].cycles_executed = 0;
	nds->dma[1].cycles_executed = 0;
	nds->exec_out = out;
	schedule_event(nds, scheduler::EXECUTION_TARGET_REACHED, target);
}

static void
run_loop(nds_ctx *nds)
{
	u64 start_cycles = nds->arm_cycles[0];
	nds->execution_finished = nds->shutdown;

	while (!nds->execution_finished) {
		nds->arm_target_cycles[0] = get_next_event_time(nds);
		if (nds->arm9->stopped()) {
			nds->arm_cycles[0] = nds->arm_target_cycles[0];
		} else if (nds->dma[0].active) {
			run_dma9(nds);
		} else {
			nds->arm9->run();
		}

		run_cpu_events(nds, 0);

		timestamp arm7_target = nds->arm_cycles[0] >> 1;
		while (nds->arm_cycles[1] < arm7_target) {
			nds->arm_target_cycles[1] = arm7_target;
			if (nds->arm7->stopped()) {
				nds->arm_cycles[1] = nds->arm_target_cycles[1];
			} else if (nds->dma[1].active) {
				run_dma7(nds);
			} else {
				nds->arm7->run();
			}

			run_cpu_events(nds, 1);
		}

		run_system_events(nds);
	}

	if (nds->exec_out) {
		nds->exec_out->cycles = nds->arm_cycles[0] - start_cycles;
		nds->exec_out->audio_buf = nds->audio_buf.data();
		nds->exec_out->audio_buf_len = nds->audio_buf_idx >> 1;
		nds->exec_out->fb = nds->fb;
		nds->exec_out->sig_flags = nds->raised_sigs;
		nds->exec_out->cpu_usage = {
			nds->cpu[0]->cycles_executed / 1120380.0,
			nds->cpu[1]->cycles_executed / 560190.0,
		};
		nds->exec_out->dma_usage = {
			nds->dma[0].cycles_executed / 1120380.0,
			nds->dma[1].cycles_executed / 560190.0,
		};
	}
}

static void
check_lyc(nds_ctx *nds, int cpuid)
{
	auto& dispstat = nds->dispstat[cpuid];
	u16 lyc = (dispstat & 0x80) << 1 | dispstat >> 8;

	if (nds->vcount == lyc) {
		dispstat |= BIT(2);
		if (dispstat & BIT(5)) {
			request_interrupt(nds->cpu[cpuid], 2);
		}
	} else {
		dispstat &= ~BIT(2);
	}
}

static void
nds_on_vblank(nds_ctx *nds)
{
	nds->frames++;

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
	check_should_savefile_flush(nds);

	if (nds->term_sigs & nds_signal::VBLANK) {
		nds->raised_sigs |= nds_signal::VBLANK;
		nds->execution_finished = true;
	}
}

static void
schedule_32k_tick_event(nds_ctx *nds, timestamp late)
{
	u32 numer = NDS_ARM9_CLK_RATE + nds->timer_32k_last_err;
	u32 denom = 32768;
	nds->timer_32k_last_period = numer / denom;
	nds->timer_32k_last_err = numer % denom;

	schedule_event_after(nds, scheduler::TICK_32KHZ,
			nds->timer_32k_last_period - late);
}

} // namespace twice
