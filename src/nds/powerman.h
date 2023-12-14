#ifndef TWICE_POWERMAN_H
#define TWICE_POWERMAN_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct powerman {
	u8 ctrl{};
	u8 battery_status{};
	u8 mic_amp_enabled{};
	u8 mic_amp_gain{};
	u8 reg_select{};
	bool read_mode{};
	bool cs_active{};
};

void powerman_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active);
void powerman_spi_reset(nds_ctx *nds);

} // namespace twice

#endif
