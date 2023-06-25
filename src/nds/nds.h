#ifndef TWICE_NDS_H
#define TWICE_NDS_H

#include "nds/dma.h"
#include "nds/gpu/gpu.h"
#include "nds/gpu/vram.h"
#include "nds/ipc.h"
#include "nds/scheduler.h"

#include "common/types.h"

namespace twice {

enum MemorySizes : u32 {
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
	MAX_CART_SIZE = 512_MiB,
};

struct Arm;
struct Arm9;
struct Arm7;

struct NDS {
	NDS(u8 *, u8 *, u8 *, u8 *, size_t);
	~NDS();

	/*
	 * HW
	 */
	Arm *cpu[2]{};
	std::unique_ptr<Arm9> arm9;
	std::unique_ptr<Arm7> arm7;

	GpuVram vram;
	Gpu2D gpu2D[2];

	Scheduler scheduler;
	u64 arm_target_cycles[2]{};
	u64 arm_cycles[2]{};

	Dma *dma[2]{ &dma9, &dma7 };
	Dma9 dma9;
	Dma7 dma7;

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
	u8 shared_wram_null[4]{};

	u8 *arm7_bios{};
	u8 *arm9_bios{};
	u8 *firmware{};
	u8 *cartridge{};
	size_t cartridge_size{};

	u32 fb[NDS_FB_SZ_BYTES]{};

	/*
	 * IO
	 */
	u16 vcount{};

	u8 vramstat{};
	u8 wramcnt{};
	u16 dispstat[2]{};

	u16 ipcsync[2]{};
	IpcFifo ipcfifo[2];

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
	u8 postflg[2]{};

	u32 soundbias{};

	u16 keyinput{ 0x3FF };
	u16 extkeyin{ 0x7F };

	/*
	 * MISC
	 */
	bool frame_finished{};
	bool trace{};
};

void nds_direct_boot(NDS *nds);
void nds_run_frame(NDS *nds);

void event_hblank_start(NDS *nds);
void event_hblank_end(NDS *nds);

} // namespace twice

#endif
