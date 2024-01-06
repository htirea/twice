#ifndef TWICE_WIFI_H
#define TWICE_WIFI_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct nds_ctx;

enum : u32 {
	WIFI_RAM_SIZE = 0x2000,
	WIFI_RAM_MASK = 0x2000 - 1,
	WIFI_IO_REG_SIZE = 0x1000,
	WIFI_IO_REG_MASK = 0x1000 - 1,
};

struct wifi {
	u8 bb_write{};
	u8 bb_read{};
	u8 bb_mode{};
	u8 bb_power{};
	u8 bb_regs[0x100]{};
	u8 wifi_regs[WIFI_IO_REG_SIZE]{};
	u16 powerstate{ BIT(9) };
	u8 ram[WIFI_RAM_SIZE];
};

template <typename T>
T io_wifi_read(nds_ctx *nds, u32 offset);
template <typename T>
void io_wifi_write(nds_ctx *nds, u32 offset, T value);

} // namespace twice

#endif
