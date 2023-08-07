#include "nds/nds.h"

namespace twice {

void
touchscreen_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& ts = nds->touchscreen;

	nds->spidata_r = 0x50;
	ts.cs_active = keep_active;
}

void
touchscreen_spi_reset(nds_ctx *nds)
{
	auto& ts = nds->touchscreen;
	ts.cs_active = false;
}

} // namespace twice
