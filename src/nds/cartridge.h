#ifndef TWICE_CARTRIDGE_H
#define TWICE_CARTRIDGE_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct cartridge {
	cartridge(u8 *data, size_t size);

	u8 *data{};
	size_t size{};
	size_t read_mask{};
	u32 chip_id{};

	struct rom_transfer {
		u8 command[8]{};
		u32 transfer_size{};
		u32 bytes_read{};
		u32 bus_data_r{};

		u32 start_addr{};
	} transfer;
};

u32 read_cart_bus_data(nds_ctx *nds, int cpuid);
void cartridge_start_command(nds_ctx *nds, int cpuid);
void event_advance_rom_transfer(nds_ctx *nds);

} // namespace twice

#endif
