#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

static u8 powerman_read_reg(nds_ctx *nds);
static void powerman_write_reg(nds_ctx *nds, u8 value);

void
powerman_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& pwr = nds->pwr;

	if (!pwr.cs_active) {
		pwr.read_mode = value & BIT(7);
		pwr.reg_select = value & 3;
	} else {
		if (pwr.read_mode) {
			nds->spidata_r = powerman_read_reg(nds);
		} else {
			powerman_write_reg(nds, value);
		}
	}

	pwr.cs_active = keep_active;
}

void
powerman_spi_reset(nds_ctx *nds)
{
	auto& pwr = nds->pwr;
	pwr.cs_active = false;
}

static u8
powerman_read_reg(nds_ctx *nds)
{
	auto& pwr = nds->pwr;

	switch (pwr.reg_select) {
	case 0:
		return pwr.ctrl;
	case 1:
		return pwr.battery_status;
	case 2:
		return pwr.mic_amp_enabled;
	default:
		return pwr.mic_amp_gain;
	}
}

static void
powerman_write_reg(nds_ctx *nds, u8 value)
{
	auto& pwr = nds->pwr;

	switch (pwr.reg_select) {
	case 0:
		if (value & BIT(6)) {
			nds->shutdown = true;
		}
		pwr.ctrl |= (value & 0x7F);
		break;
	case 1:
		break;
	case 2:
		pwr.mic_amp_enabled |= value & 1;
		break;
	case 3:
		pwr.mic_amp_gain |= value & 3;
	}
}

} // namespace twice
