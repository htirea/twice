#ifndef TWICE_SPI_H
#define TWICE_SPI_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

u8 spidata_read(nds_ctx *nds);
void spidata_write(nds_ctx *nds, u8 value);
void spicnt_write(nds_ctx *nds, u16 value);
void event_spi_transfer_complete(nds_ctx *nds, intptr_t, timestamp);

} // namespace twice

#endif
