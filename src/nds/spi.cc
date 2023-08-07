#include "nds/spi.h"

#include "nds/arm/arm.h"
#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

u8
spidata_read(nds_ctx *nds)
{
	return nds->spidata_r;
}

void
spidata_write(nds_ctx *nds, u8 value)
{
	if (!(nds->spicnt & BIT(15))) {
		return;
	}

	nds->spidata_w = value;

	u16 device = nds->spicnt >> 8 & 3;
	bool keep_active = nds->spicnt & BIT(11);

	switch (device) {
	case 0:
	case 3:
		LOG("spi write device %d pc %08X\n", device,
				nds->cpu[1]->pc());
		return;
	case 1:
		firmware_spi_transfer_byte(nds, value, keep_active);
		break;
	case 2:
		touchscreen_spi_transfer_byte(nds, value, keep_active);
		break;
	}

	schedule_cpu_event_after(nds, 1,
			event_scheduler::SPI_TRANSFER_COMPLETE,
			64 << (nds->spicnt & 3));
	nds->spicnt |= BIT(7);
}

void
spicnt_write(nds_ctx *nds, u16 value)
{
	nds->spicnt = (nds->spicnt & ~0xCF03) | (value & 0xCF03);
}

void
event_spi_transfer_complete(nds_ctx *nds, [[maybe_unused]] int cpuid,
		[[maybe_unused]] intptr_t data)
{
	nds->spicnt &= ~BIT(7);
	if (nds->spicnt & BIT(14)) {
		request_interrupt(nds->cpu[1], 23);
	}
}

} // namespace twice
