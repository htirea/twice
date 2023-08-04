#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

constexpr unsigned CS_WRITE = BIT(6);
constexpr unsigned SCK_WRITE = BIT(5);
constexpr unsigned DATA_WRITE = BIT(4);
constexpr unsigned CS = BIT(2);
constexpr unsigned SCK = BIT(1);
constexpr unsigned DATA = BIT(0);

u8
rtc_read(nds_ctx *nds)
{
	u8 cr = nds->rtc.clock_reg & ~1;
	return cr & DATA_WRITE ? cr | nds->rtc.input_bit
	                       : cr | nds->rtc.output_bit;
}

static void
init_new_transfer(real_time_clock *rtc)
{
	rtc->output_bytes.clear();
	rtc->output_byte_num = 0;
	rtc->output_bit_num = 0;
	rtc->output_bit = 0;
	rtc->input_bytes.clear();
	rtc->param_bytes_left = 0;
	rtc->input_byte = 0;
	rtc->input_bit_num = 0;
	rtc->input_bit = 0;
}

static u8
reverse_bits(u8 x)
{
	x = (x & 0xF) << 4 | (x & 0xF0) >> 4;
	x = (x & 0x33) << 2 | (x & 0xCC) >> 2;
	x = (x & 0x55) << 1 | (x & 0xAA) >> 1;
	return x;
}

static void
process_command_byte(real_time_clock *rtc)
{
	if (rtc->input_byte >> 4 != 6) {
		rtc->input_byte = reverse_bits(rtc->input_byte);
	}
	if (rtc->input_byte >> 4 != 6) {
		LOG("[rtc] invalid command byte %04X\n", rtc->input_byte);
		rtc->input_byte = 0x60;
	}

	if (rtc->input_byte & 1) {
		return;
	}

	switch (rtc->input_byte >> 1 & 7) {
	case 0:
	case 1:
	case 6:
	case 7:
		rtc->param_bytes_left = 1;
		break;
	case 2:
		rtc->param_bytes_left = 7;
		break;
	case 3:
	case 5:
		rtc->param_bytes_left = 3;
		break;
	case 4:
		LOG("command byte 0x64\n");
		rtc->param_bytes_left = 1;
		break;
	}
}

static void
command_read_params(real_time_clock *rtc)
{
	rtc->output_bytes.clear();
	rtc->output_byte_num = 0;
	rtc->output_bit_num = 0;

	switch (rtc->input_byte >> 1 & 7) {
	case 0:
		rtc->output_bytes.push_back(rtc->stat1);
		break;
	case 1:
		rtc->output_bytes.push_back(rtc->stat2);
		break;
	case 2:
		rtc->output_bytes.insert(rtc->output_bytes.end(),
				{ 0x23, 0x8, 0x4, 0x4, 0x3, 0x51, 0x11 });
		break;
	default:
		LOG("rtc command read %02X\n", rtc->input_byte);
	}
}

static void
command_write_params(real_time_clock *rtc)
{
	LOG("rtc command: ");
	for (u8 p : rtc->input_bytes) {
		LOG("%02X ", p);
	}
	LOG("\n");
}

static void
send_bit(real_time_clock *rtc, bool data)
{
	rtc->input_bit = data;
	rtc->input_byte |= data << rtc->input_bit_num;
	rtc->input_bit_num++;
	if (rtc->input_bit_num == 8) {
		rtc->input_bit_num = 0;

		if (rtc->input_bytes.size() == 0) {
			process_command_byte(rtc);
			if (rtc->input_byte & 1) {
				command_read_params(rtc);
			}
		} else {
			rtc->input_bytes.push_back(rtc->input_byte);
			rtc->param_bytes_left--;
			if (rtc->param_bytes_left == 0) {
				command_write_params(rtc);
			}
		}
	}
}

static void
recv_bit(real_time_clock *rtc)
{
	if (rtc->output_byte_num >= rtc->output_bytes.size()) {
		rtc->output_bit = 0;
		return;
	}

	u8 output_byte = rtc->output_bytes[rtc->output_byte_num];
	rtc->output_bit = output_byte >> rtc->output_bit_num & 1;
	rtc->output_bit_num++;
	if (rtc->output_bit_num == 8) {
		rtc->output_bit_num = 0;
		rtc->output_byte_num++;
	}
}

void
rtc_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	u8 old = rtc.clock_reg;

	rtc.clock_reg &= ~(CS_WRITE | SCK_WRITE | DATA_WRITE);
	rtc.clock_reg |= value & (CS_WRITE | SCK_WRITE | DATA_WRITE);

	if (value & CS_WRITE) {
		if (!(old & CS) && (value & CS)) {
			init_new_transfer(&rtc);
		}
		rtc.clock_reg = (rtc.clock_reg & ~CS) | (value & CS);
	}

	if (value & SCK_WRITE) {
		if ((old & SCK) && !(value & SCK)) {
			if (value & DATA_WRITE) {
				send_bit(&rtc, value & DATA);
			} else {
				recv_bit(&rtc);
			}
		}
		rtc.clock_reg = (rtc.clock_reg & ~SCK) | (value & SCK);
	}

	if (value & DATA_WRITE) {
		rtc.clock_reg = (rtc.clock_reg & ~DATA) | (value & DATA);
	}
}

} // namespace twice
