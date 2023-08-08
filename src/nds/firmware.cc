#include "nds/nds.h"

#include "common/logger.h"

#include "libtwice/exception.h"

namespace twice {

static u16
calculate_crc16(u8 *p, size_t num_bytes)
{
	u16 v[] = { 0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001,
		0xA001 };

	u32 crc = 0xFFFF;
	for (size_t i = 0; i < num_bytes; i++) {
		crc ^= p[i];
		for (int j = 0; j < 8; j++) {
			bool carry = crc & 1;
			crc >>= 1;
			if (carry) {
				crc ^= v[j] << (7 - j);
			}
		}
	}

	return crc;
}

firmware_flash::firmware_flash(u8 *data)
	: data(data)
{
	u32 user_settings_offset = readarr<u16>(data, 0x20) << 3;
	if (user_settings_offset != 0x3FE00) {
		throw twice_error("unhandled firmware user settings offset");
	}

	user_settings = data + user_settings_offset;
	writearr<u16>(user_settings, 0x58, 0);
	writearr<u16>(user_settings, 0x5A, 0);
	writearr<u8>(user_settings, 0x5C, 0);
	writearr<u8>(user_settings, 0x5D, 0);
	writearr<u16>(user_settings, 0x5E, 255 << 4);
	writearr<u16>(user_settings, 0x60, 191 << 4);
	writearr<u8>(user_settings, 0x62, 255);
	writearr<u8>(user_settings, 0x63, 191);

	u16 checksum = calculate_crc16(user_settings, 0x70);
	writearr<u16>(user_settings, 0x72, checksum);
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
get_3_byte_address(u8 *bytes)
{
	return (u32)bytes[0] << 16 | (u32)bytes[1] << 8 | (u32)bytes[2];
}

static void
on_all_params_received(nds_ctx *nds)
{
	auto& fw = nds->firmware;

	switch (fw.command) {
	case 0x03:
	{
		fw.addr = get_3_byte_address(fw.input_bytes.data());
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

	fw.cs_active = keep_active;
}

void
firmware_spi_reset(nds_ctx *nds)
{
	auto& fw = nds->firmware;
	fw.cs_active = false;
}

} // namespace twice
