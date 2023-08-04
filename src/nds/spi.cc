#include "nds/spi.h"

#include "nds/nds.h"

namespace twice {

u8
spidata_read(nds_ctx *nds)
{
	return nds->spidata_r;
}

void
spidata_write(nds_ctx *nds, u8 value)
{
	nds->spidata_w = value;
}

void
spicnt_write(nds_ctx *nds, u16 value)
{
	u16 old = nds->spicnt;
	nds->spicnt = (nds->spicnt & ~0xCF03) & (value & 0xCF03);
	if (!(old & BIT(15)) && (value & BIT(15))) {
		nds->spicnt &= ~BIT(11);
	}
}

} // namespace twice
