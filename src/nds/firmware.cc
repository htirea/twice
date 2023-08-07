#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

firmware_flash::firmware_flash(u8 *data)
	: data(data)
{
}

static void
init_new_transfer(nds_ctx *nds)
{
	auto& fw = nds->firmware;

	fw.state = 0;
	fw.command = 0;
	fw.num_params = 0;
	fw.cs_active = true;
	fw.input_bytes.clear();
}

static void
start_command(nds_ctx *nds, u8 command)
{
	auto& fw = nds->firmware;

	fw.command = command;
	switch (command) {
	case 0x03:
		fw.num_params = 3;
		fw.num_params_left = 3;
		break;
	case 0x05:
		fw.num_params = 0;
		fw.num_params_left = 0;
		break;
	default:
		LOG("[firmware] unhandled command %02X\n", command);
	}
}

static u32
get_3_byte_address(nds_ctx *nds)
{
	auto& bytes = nds->firmware.input_bytes;
	return (u32)bytes[0] << 16 | (u32)bytes[1] << 8 | (u32)bytes[2];
}

static void
on_all_params_received(nds_ctx *nds)
{
	auto& fw = nds->firmware;

	switch (fw.command) {
	case 0x03:
	{
		fw.addr = get_3_byte_address(nds);
		break;
	}
	}
}

static void
continue_command(nds_ctx *nds, u8 value)
{
	auto& fw = nds->firmware;

	if (fw.num_params_left > 0) {
		fw.input_bytes.push_back(value);
		fw.num_params_left--;
		if (fw.num_params_left == 0) {
			on_all_params_received(nds);
		}
	} else {
		switch (fw.command) {
		case 0x03:
			nds->spidata_r = readarr<u8>(
					fw.data, fw.addr & FIRMWARE_MASK);
			fw.addr++;
			break;
		case 0x05:
			nds->spidata_r = fw.status;
			break;
		}
	}
}

void
firmware_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& fw = nds->firmware;

	if (!fw.cs_active) {
		init_new_transfer(nds);
		start_command(nds, value);
	} else {
		continue_command(nds, value);
	}

	if (keep_active) {
		fw.cs_active = true;
	} else {
		fw.cs_active = false;
	}
}

} // namespace twice
