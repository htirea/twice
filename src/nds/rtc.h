#ifndef TWICE_RTC_H
#define TWICE_RTC_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct real_time_clock {
	u8 clock_reg{};
	u8 stat1{ 0x2 };
	u8 stat2{};
	int year{ 2000 };
	int month{ 1 };
	int day{ 1 };
	int weekday{ 5 };
	int hour{ 0 };
	int minute{ 0 };
	int second{ 0 };

	std::vector<u8> output_bytes;
	size_t output_byte_num{};
	int output_bit_num{};
	bool output_bit{};

	std::vector<u8> input_bytes;
	int param_bytes_left{};
	u8 input_byte{};
	int input_bit_num{};
	bool input_bit{};
};

u8 rtc_read(nds_ctx *nds);
void rtc_write(nds_ctx *nds, u8 value);
void event_rtc_tick(nds_ctx *nds, intptr_t, timestamp late);

} // namespace twice

#endif
