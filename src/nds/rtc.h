#ifndef TWICE_RTC_H
#define TWICE_RTC_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct real_time_clock {
	u8 year{ 0 };
	u8 month{ 1 };
	u8 day{ 1 };
	u8 weekday{ 5 };
	u8 hour{ 0 };
	u8 minute{ 0 };
	u8 second{ 0 };
	u8 stat1{ 0x40 };
	u8 stat2{};
	u8 intreg[2][3]{};
	u8 clock_correction{};
	u8 free_reg{};
	u8 io_reg{};
	bool cs{};
	bool sck{};
	bool sio_out{};
	bool irq_out[2]{};
	int params_left{};
	u8 cmd_byte{};
	int interrupt_mode{};

	std::queue<bool> input_bits;
	std::vector<u8> cmd_params;
	std::queue<bool> output_bits;
};

u8 rtc_io_read(nds_ctx *nds);
void rtc_io_write(nds_ctx *nds, u8 value);
void rtc_tick_32k(nds_ctx *nds);

} // namespace twice

#endif
