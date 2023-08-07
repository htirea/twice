#ifndef TWICE_TOUCHSCREEN_H
#define TWICE_TOUCHSCREEN_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct touchscreen_controller {
	bool cs_active{};
};

void touchscreen_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active);
void touchscreen_spi_reset(nds_ctx *nds);

} // namespace twice

#endif
