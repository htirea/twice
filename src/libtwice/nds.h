#ifndef LIBTWICE_NDS_H
#define LIBTWICE_NDS_H

#include "libtwice/types.h"

namespace twice {

enum MemorySizes : u32 {
	ITCM_SIZE = 32_KiB,
	ITCM_MASK = 32_KiB - 1,
	DTCM_SIZE = 16_KiB,
	DTCM_MASK = 16_KiB - 1,
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

struct Arm9;
struct Arm7;

struct NDS {
	NDS(u8 *, u8 *, u8 *, u8 *, size_t);
	~NDS();

	std::unique_ptr<Arm9> arm9;
	std::unique_ptr<Arm7> arm7;

	u8 *arm7_bios{};
	u8 *arm9_bios{};
	u8 *firmware{};
	u8 *cartridge{};
	size_t cartridge_size{};

	void direct_boot();
};

} // namespace twice

#endif
