#include "nds/nds.h"

#include "common/logger.h"

#include "libtwice/exception.h"

namespace twice {

static u16 calculate_crc16(u8 *p, size_t length);

void
firmware_init(nds_ctx *nds)
{
	auto& fw = nds->fw;
	fw.data = nds->firmware_v.data();

	u32 user_settings_offset = readarr<u16>(fw.data, 0x20) << 3;
	if (user_settings_offset != 0x3FE00) {
		throw twice_error("unhandled firmware user settings offset");
	}

	fw.user_settings = fw.data + user_settings_offset;

	/* overwrite touchscreen calibration data */
	writearr<u16>(fw.user_settings, 0x58, 0);
	writearr<u16>(fw.user_settings, 0x5A, 0);
	writearr<u8>(fw.user_settings, 0x5C, 0);
	writearr<u8>(fw.user_settings, 0x5D, 0);
	writearr<u16>(fw.user_settings, 0x5E, 255 << 4);
	writearr<u16>(fw.user_settings, 0x60, 191 << 4);
	writearr<u8>(fw.user_settings, 0x62, 255);
	writearr<u8>(fw.user_settings, 0x63, 191);

	u16 checksum = calculate_crc16(fw.user_settings, 0x70);
	writearr<u16>(fw.user_settings, 0x72, checksum);
}

void
firmware_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& fw = nds->fw;

	if (!fw.cs_active) {
		fw.command = value;
		fw.count = 0;
	}

	switch (fw.command) {
	case 0x3:
		switch (fw.count) {
		case 0:
			break;
		case 1:
			fw.addr = (u32)value << 16;
			break;
		case 2:
			fw.addr |= (u32)value << 8;
			break;
		case 3:
			fw.addr |= value;
			break;
		default:
			nds->spidata_r = readarr<u8>(
					fw.data, fw.addr & FIRMWARE_MASK);
			fw.addr++;
		}
		break;
	case 0x5:
		if (fw.count > 0) {
			nds->spidata_r = fw.stat_reg;
		}
		break;
	default:
		LOG("[firmware] unhandled command %02X\n", fw.command);
	}

	fw.count++;
	fw.cs_active = keep_active;
}

void
firmware_spi_reset(nds_ctx *nds)
{
	auto& fw = nds->fw;
	fw.cs_active = false;
}

static u16
calculate_crc16(u8 *p, size_t length)
{
	u16 v[] = { 0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001,
		0xA001 };
	u32 crc = 0xFFFF;

	for (size_t i = 0; i < length; i++) {
		crc ^= p[i];
		for (u32 j = 0; j < 8; j++) {
			bool carry = crc & 1;
			crc >>= 1;
			if (carry) {
				crc ^= v[j] << (7 - j);
			}
		}
	}

	return crc;
}

} // namespace twice
