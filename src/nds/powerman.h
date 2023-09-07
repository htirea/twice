#ifndef TWICE_POWERMAN_H
#define TWICE_POWERMAN_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct power_management_device {
	u8 reg[4]{};
	u8 output_byte{};
	u8 reg_select{};
	bool read_mode{};
	bool cs_active{};
};

void powerman_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active);
void powerman_spi_reset(nds_ctx *nds);

} // namespace twice

#endif
