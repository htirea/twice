#ifndef LIBTWICE_NDS_H
#define LIBTWICE_NDS_H

#include "libtwice/types.h"

namespace twice {

constexpr size_t NDS_BIOS7_SIZE = 16_KiB;
constexpr size_t NDS_BIOS9_SIZE = 4_KiB;
constexpr size_t NDS_FIRMWARE_SIZE = 256_KiB;
constexpr size_t NDS_MAX_CART_SIZE = 512_MiB;

struct NDS {
	NDS(u8 *, u8 *, u8 *, u8 *, size_t);
	~NDS();

	int direct_boot();

	u8 *arm7_bios{};
	u8 *arm9_bios{};
	u8 *firmware{};
	u8 *cartridge{};
	size_t cartridge_size{};
};

} // namespace twice

#endif
