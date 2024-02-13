#ifndef TWICE_CART_H
#define TWICE_CART_H

#include "common/types.h"

#include "nds/cart/backup.h"

namespace twice {

struct nds_ctx;

struct cartridge {
	u8 *data{};
	size_t size{};
	size_t read_mask{};
	u32 chip_id{};
	u32 gamecode{};
	bool infrared{};
	cartridge_backup backup;

	struct {
		u8 command[8]{};
		u32 length{};
		u32 count{};
		u32 addr{};
		u32 bus_data_r{};
		bool key1{};
	} transfer;

	u32 keybuf[0x412]{};
	u32 keybuf_s[0x412]{};
	u8 keycode[16]{};
};

void cartridge_init(nds_ctx *nds, int savetype);
void parse_cart_header(nds_ctx *nds, u32 *entry_addr_out);
void romctrl_write(nds_ctx *nds, int cpuid, u32 value);
u32 read_cart_bus_data(nds_ctx *nds, int cpuid);
void event_advance_rom_transfer(nds_ctx *nds, intptr_t, timestamp late);

} // namespace twice

#endif
