#ifndef TWICE_FIRMWARE_H
#define TWICE_FIRMWARE_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct firmware_flash {
	firmware_flash(u8 *data);

	u8 *data{};
	u8 *user_settings{};
	int state{};
	u8 command{};
	int num_params{};
	int num_params_left{};
	bool cs_active{};
	std::vector<u8> input_bytes;

	u32 addr{};
	u8 status{};
};

void firmware_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active);
void firmware_spi_reset(nds_ctx *nds);

} // namespace twice

#endif
