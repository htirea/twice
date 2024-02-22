#ifndef TWICE_NDS_H
#define TWICE_NDS_H

#include "libtwice/file/file.h"
#include "libtwice/nds/defs.h"
#include "libtwice/nds/game_db.h"
#include "libtwice/nds/machine.h"

#include "nds/cart/cart.h"
#include "nds/dma.h"
#include "nds/firmware.h"
#include "nds/gpu/2d/gpu2d.h"
#include "nds/gpu/3d/gpu3d.h"
#include "nds/gpu/vram.h"
#include "nds/ipc.h"
#include "nds/powerman.h"
#include "nds/rtc.h"
#include "nds/scheduler.h"
#include "nds/sound.h"
#include "nds/timer.h"
#include "nds/touchscreen.h"
#include "nds/wifi.h"

#include "common/profiler.h"
#include "common/types.h"

namespace twice {

enum class run_mode {
	RUN_FOR,
	RUN_UNTIL,
	RUN_UNTIL_VBLANK,
};

enum : u32 {
	MAIN_RAM_SIZE = 4_MiB,
	MAIN_RAM_MASK = 4_MiB - 1,
	SHARED_WRAM_SIZE = 32_KiB,
	SHARED_WRAM_MASK = 32_KiB - 1,
	PALETTE_SIZE = 2_KiB,
	PALETTE_MASK = 2_KiB - 1,
	OAM_SIZE = 2_KiB,
	OAM_MASK = 2_KiB - 1,
	ARM9_BIOS_SIZE = 4_KiB,
	ARM9_BIOS_MASK = 4_KiB - 1,
	ARM7_BIOS_SIZE = 16_KiB,
	ARM7_BIOS_MASK = 16_KiB - 1,
	ARM7_WRAM_SIZE = 64_KiB,
	ARM7_WRAM_MASK = 64_KiB - 1,
	FIRMWARE_SIZE = 256_KiB,
	FIRMWARE_MASK = 256_KiB - 1,
	WIFI_REGION_SIZE = 32_KiB,
	WIFI_REGION_MASK = 32_KiB - 1,
	MIN_CART_SIZE = 0x160,
	MAX_CART_SIZE = 512_MiB,
};

struct arm_cpu;
struct arm9_cpu;
struct arm7_cpu;

struct nds_ctx {
	~nds_ctx();

	/*
	 * HW
	 */
	arm_cpu *cpu[2]{};
	std::unique_ptr<arm9_cpu> arm9;
	std::unique_ptr<arm7_cpu> arm7;

	gpu_vram vram;
	gpu_2d_engine gpu2d[2];
	gpu_3d_engine gpu3d;

	scheduler sc;
	timestamp arm_target_cycles[2]{};
	timestamp arm_cycles[2]{};

	dma_controller dma[2];
	timer tmr[2][4];
	real_time_clock rtc;
	firmware fw;
	touchscreen ts;
	cartridge cart;
	powerman pwr;
	sound_channel sound_ch[16];
	sound_capture_channel sound_cap_ch[2];
	wifi wf;

	u32 timer_32k_ticks{};
	u32 timer_32k_last_period{};
	u32 timer_32k_last_err{};

	/*
	 * Memory
	 */
	u8 main_ram[MAIN_RAM_SIZE]{};
	u8 shared_wram[SHARED_WRAM_SIZE]{};
	u8 palette[PALETTE_SIZE]{};
	u8 oam[OAM_SIZE]{};
	u8 arm7_wram[ARM7_WRAM_SIZE]{};

	u8 *shared_wram_p[2]{};
	u32 shared_wram_mask[2]{};
	u8 shared_wram_null[16_KiB]{};

	u8 *arm7_bios{};
	u8 *arm9_bios{};

	u32 fb[NDS_FB_SZ]{};
	std::array<s16, 4096> audio_buf{};
	u32 audio_buf_idx{};
	std::array<s16, 4096> mic_buf{};
	u32 mic_buf_idx{};

	/*
	 * IO
	 */
	u16 vcount{};

	u8 vramstat{};
	u8 wramcnt{};
	u16 dispstat[2]{};

	u16 ipcsync[2]{};
	ipc_fifo ipcfifo[2];

	u16 sqrtcnt{};
	u32 sqrt_result{};
	u32 sqrt_param[2]{};

	u16 divcnt{};
	u32 div_numer[2]{};
	u32 div_denom[2]{};
	u32 div_result[2]{};
	u32 divrem_result[2]{};

	u32 dma_sad[2][4]{};
	u32 dma_dad[2][4]{};
	u16 dmacnt_l[2][4]{};
	u16 dmacnt_h[2][4]{};
	u32 dmafill[4]{};

	u8 haltcnt{};
	u16 powcnt1{};
	u8 powcnt2{ 0x1 };
	u8 wifiwaitcnt{};
	u8 postflg[2]{};

	u16 soundcnt{};
	u32 soundbias{};

	u16 keyinput{ 0x3FF };
	u16 extkeyin{ 0x7F };
	u16 rcnt{};

	u16 exmem[2]{ 0x6000, 0x6000 };
	int nds_slot_cpu{};
	int gba_slot_cpu{};

	u16 auxspicnt{};
	u8 auxspidata_r{};
	u32 romctrl{};
	u8 cart_command_out[8]{};
	u32 encryption_seed_l[2]{};
	u16 encryption_seed_h[2]{};

	u16 spicnt{};
	u8 spidata_r{};

	/*
	 * MISC
	 */
	bool execution_finished{};
	bool trace{};
	bool shutdown{};
	double arm9_usage{};
	double arm7_usage{};
	double dma9_usage{};
	double dma7_usage{};
	unsigned long term_sigs{};
	unsigned long raised_sigs{};

	profiler prof;
	nds_config *config{};
	nds_exec *exec_out{};
	fs::file_view arm9_bios_v;
	fs::file_view arm7_bios_v;
	fs::file_view firmware_v;
	fs::file_view cart_v;
	fs::file_view save_v;
};

std::unique_ptr<nds_ctx> create_nds_ctx(fs::file_view arm9_bios,
		fs::file_view arm7_bios, fs::file_view firmware,
		fs::file_view cart, fs::file_view save, nds_savetype savetype,
		nds_config *config);
void nds_firmware_boot(nds_ctx *nds);
void nds_direct_boot(nds_ctx *nds);
void nds_run(nds_ctx *nds, run_mode mode, const nds_exec *in, nds_exec *out);
void nds_set_rtc_state(nds_ctx *nds, const nds_rtc_state& s);
void nds_set_touchscreen_state(nds_ctx *nds, int x, int y, bool down,
		bool quicktap, bool moved);
void nds_sync_files(nds_ctx *nds);
void nds_dump_prof(nds_ctx *nds);

void event_execution_target_reached(nds_ctx *nds, intptr_t, timestamp late);
void event_hblank_start(nds_ctx *nds, intptr_t, timestamp late);
void event_hblank_end(nds_ctx *nds, intptr_t, timestamp late);
void event_32khz_tick(nds_ctx *nds, intptr_t, timestamp late);

} // namespace twice

#endif
