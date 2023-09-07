#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

static void
powerman_write_reg(nds_ctx *nds, u8 value)
{
	auto& pwr = nds->powerman;

	switch (pwr.reg_select) {
	case 0:
		pwr.reg[0] = (pwr.reg[0] & ~0x7F) | (value & 0x7F);
		if (value & BIT(6)) {
			nds->shutdown = true;
		}
		break;
	default:
		LOGV("powerman write to reg %d, value %02X\n", pwr.reg_select,
				value);
	}
}

void
powerman_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& pwr = nds->powerman;

	nds->spidata_r = pwr.output_byte;

	if (!pwr.cs_active) {
		pwr.read_mode = value & 0x80;

		if (pwr.read_mode) {
			pwr.output_byte = pwr.reg[value & 3];
		} else {
			pwr.reg_select = value & 3;
		}
	} else {
		if (!pwr.read_mode) {
			powerman_write_reg(nds, value);
		}
	}

	pwr.cs_active = keep_active;
}

void
powerman_spi_reset(nds_ctx *nds)
{
	auto& pwr = nds->powerman;
	pwr.cs_active = false;
}

} // namespace twice
