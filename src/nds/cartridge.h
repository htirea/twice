#ifndef TWICE_CARTRIDGE_H
#define TWICE_CARTRIDGE_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct cartridge {
	cartridge(u8 *data, size_t size);

	u8 *data{};
	size_t size{};

	s32 bus_transfer_bytes_left{};
};

u32 read_cart_bus_data(nds_ctx *nds, int cpuid);
void cartridge_start_command(nds_ctx *nds, int cpuid);
u32 cartridge_make_chip_id(nds_ctx *nds);

} // namespace twice

#endif
