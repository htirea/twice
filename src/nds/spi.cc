#include "nds/spi.h"

#include "nds/arm/arm.h"
#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

static void reset_spi(nds_ctx *);

u8
spidata_read(nds_ctx *nds)
{
	return nds->spidata_r;
}

void
spidata_write(nds_ctx *nds, u8 value)
{
	bool enabled = nds->spicnt & BIT(15);
	bool keep_active = nds->spicnt & BIT(11);
	u16 device = nds->spicnt >> 8 & 3;

	if (!enabled)
		return;

	switch (device) {
	case 0:
		powerman_spi_transfer_byte(nds, value, keep_active);
		break;
	case 1:
		firmware_spi_transfer_byte(nds, value, keep_active);
		break;
	case 2:
		touchscreen_spi_transfer_byte(nds, value, keep_active);
		break;
	case 3:
		LOG("spi write device 3 at pc %08X\n", nds->cpu[1]->pc());
		return;
	}

	schedule_event_after(nds, 1, scheduler::ARM7_SPI_TRANSFER,
			64 << (nds->spicnt & 3) << 1);
	nds->spicnt |= BIT(7);
}

void
spicnt_write(nds_ctx *nds, u16 value)
{
	bool old_enabled = nds->spicnt & BIT(15);
	bool enabled = value & BIT(15);

	nds->spicnt = (nds->spicnt & ~0xCF03) | (value & 0xCF03);
	if (!old_enabled && enabled) {
		reset_spi(nds);
	}
}

void
event_spi_transfer_complete(nds_ctx *nds, intptr_t data, timestamp late)
{
	nds->spicnt &= ~BIT(7);
	if (nds->spicnt & BIT(14)) {
		request_interrupt(nds->cpu[1], 23);
	}
}

static void
reset_spi(nds_ctx *nds)
{
	firmware_spi_reset(nds);
	touchscreen_spi_reset(nds);
	powerman_spi_reset(nds);
}

} // namespace twice
