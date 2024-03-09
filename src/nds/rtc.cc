#include "nds/nds.h"

#include "nds/arm/arm.h"

#include "common/date.h"
#include "common/logger.h"

namespace twice {

enum : u8 {
	SIO_BIT = BIT(0),
	SCK_BIT = BIT(1),
	CS_BIT = BIT(2),
	SIO_WRITE = BIT(4),
	SCK_WRITE = BIT(5),
	CS_WRITE = BIT(6),
};

enum {
	INT_NONE,
	INT_USERFREQ,
	INT_MINUTE_EDGE,
	INT_MINUTE_PERIOD1,
	INT_ALARM1,
	INT_MINUTE_PERIOD2,
	INT_32768,
};

static bool read_bit_from_rtc(nds_ctx *nds);
static void write_bit_to_rtc(nds_ctx *nds, bool bit);
static void write_byte_to_rtc(nds_ctx *nds, u8 value);
static void process_command_byte(nds_ctx *nds);
static void execute_read_command(nds_ctx *nds);
static void execute_write_command(nds_ctx *nds);
static void stat1_write(nds_ctx *nds, u8 value);
static void stat2_write(nds_ctx *nds, u8 value);
static void int1_read(nds_ctx *nds);
static void int1_write(nds_ctx *nds);
static void alarm_read(nds_ctx *nds, int id);
static void alarm_write(nds_ctx *nds, int id, u8 p0, u8 p1, u8 p2);
static void year_write(nds_ctx *nds, u8 value);
static void month_write(nds_ctx *nds, u8 value);
static void day_write(nds_ctx *nds, u8 value);
static void weekday_write(nds_ctx *nds, u8 value);
static void hour_write(nds_ctx *nds, u8 value);
static void minute_write(nds_ctx *nds, u8 value);
static void second_write(nds_ctx *nds, u8 value);
static void tick_second(nds_ctx *nds);
static void tick_minute(nds_ctx *nds);
static void tick_hour(nds_ctx *nds);
static void tick_day(nds_ctx *nds);
static void tick_month(nds_ctx *nds);
static void rtc_year(nds_ctx *nds);
static void rtc_reset(nds_ctx *nds);
static void set_irq_out(nds_ctx *nds, int id, bool value);
static void output_bits_msb_first(nds_ctx *nds, u8 value);
static void output_bits_lsb_first(nds_ctx *nds, u8 value);
static u8 to_bcd(int x);
static int from_bcd(u8 x);
static int from_bcd_checked(u8 x);
static u8 reverse_bits(u8 x);

u8
rtc_io_read(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	return rtc.io_reg;
}

void
rtc_io_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	bool falling_sck = false;

	if (value & CS_WRITE) {
		rtc.cs = value & CS_BIT;
	}

	if (value & SCK_WRITE) {
		if (rtc.cs) {
			bool old_sck = rtc.sck;
			rtc.sck = value & SCK_BIT;
			falling_sck = old_sck && !rtc.sck;
		}
	}

	if (falling_sck) {
		if (value & SIO_WRITE) {
			write_bit_to_rtc(nds, value & SIO_BIT);
		} else {
			rtc.sio_out = read_bit_from_rtc(nds);
		}
	}

	if (value & SIO_WRITE) {
		rtc.io_reg = value;
	} else {
		rtc.io_reg = (value & 0xFFFE) | rtc.sio_out;
	}
}

void
rtc_tick_32k(nds_ctx *nds)
{
	auto& rtc = nds->rtc;

	if ((nds->timer_32k_ticks & 32767) == 0) {
		tick_second(nds);
	}

	if (rtc.interrupt_mode == INT_USERFREQ) {
		u8 wave_state = reverse_bits(
				nds->timer_32k_ticks >> 10 & 0x1F);
		u8 wave_enabled = rtc.intreg[0][2] << 3;
		set_irq_out(nds, 0,
				(wave_state & wave_enabled) == wave_enabled);
	}
}

void
nds_set_rtc_state(nds_ctx *nds, const nds_rtc_state& s)
{
	auto& rtc = nds->rtc;
	bool use_24_hour = s.use_24_hour_clock;

	int year = s.year & 0xFF;
	if (year > 99) {
		year = 0;
	}

	int month = s.month & 0xFF;
	if (!(1 <= month && month <= 12)) {
		month = 1;
	}

	int day = s.day & 0xFF;
	if (!(1 <= day && day <= 31)) {
		day = 1;
	}
	int days_in_month = get_days_in_month(month, year);
	if (day > days_in_month) {
		day = 1;
		month++;
	}

	int weekday = s.weekday & 7;
	if (weekday > 6) {
		weekday = 0;
	}

	int hour = s.hour & 0xFF;
	if ((use_24_hour && hour > 23) || (!use_24_hour && hour > 11)) {
		hour = 0;
	}

	int minute = s.minute & 0xFF;
	if (minute > 59) {
		minute = 0;
	}

	int second = s.second & 0xFF;
	if (second > 59) {
		second = 0;
	}

	rtc.year = to_bcd(year);
	rtc.month = to_bcd(month);
	rtc.day = to_bcd(day);
	rtc.weekday = to_bcd(weekday);
	rtc.hour = to_bcd(hour);
	rtc.minute = to_bcd(minute);
	rtc.second = to_bcd(second);
	if (use_24_hour) {
		rtc.stat1 |= BIT(6);
	} else {
		rtc.stat1 &= ~BIT(6);
	}
}

static bool
read_bit_from_rtc(nds_ctx *nds)
{
	auto& rtc = nds->rtc;

	if (rtc.output_bits.empty()) {
		return 0;
	} else {
		bool bit = rtc.output_bits.front();
		rtc.output_bits.pop();
		return bit;
	}
}

static void
write_bit_to_rtc(nds_ctx *nds, bool bit)
{
	auto& rtc = nds->rtc;

	rtc.input_bits.push(bit);

	if (rtc.input_bits.size() == 8) {
		u8 byte = 0;

		while (!rtc.input_bits.empty()) {
			byte <<= 1;
			byte |= rtc.input_bits.front();
			rtc.input_bits.pop();
		}

		write_byte_to_rtc(nds, byte);
	}
}

static void
write_byte_to_rtc(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;

	if (rtc.params_left == 0) {
		rtc.cmd_byte = value;
		process_command_byte(nds);
	} else {
		rtc.cmd_params.push_back(value);
		rtc.params_left--;
		if (rtc.params_left == 0) {
			execute_write_command(nds);
		}
	}
}

static void
process_command_byte(nds_ctx *nds)
{
	auto& rtc = nds->rtc;

	if (rtc.cmd_byte >> 4 != 6) {
		rtc.cmd_byte = reverse_bits(rtc.cmd_byte);
	}

	if (rtc.cmd_byte >> 4 != 6) {
		LOG("[rtc] invalid command byte %02X\n", rtc.cmd_byte);
		return;
	}

	if (rtc.cmd_byte & 1) {
		execute_read_command(nds);
	} else {
		switch (rtc.cmd_byte >> 1 & 7) {
		case 0:
		case 1:
		case 6:
		case 7:
			rtc.params_left = 1;
			break;
		case 2:
			rtc.params_left = 7;
			break;
		case 3:
		case 5:
			rtc.params_left = 3;
			break;
		case 4:
			switch (rtc.interrupt_mode) {
			case INT_ALARM1:
				rtc.params_left = 3;
				break;
			case INT_USERFREQ:
				rtc.params_left = 1;
				break;
			default:
				LOG("[rtc] invalid int1 access, stat2: %02X\n",
						rtc.stat2);
			}
		}

		rtc.cmd_params.clear();
	}
}

static void
execute_read_command(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	u8 cmd = rtc.cmd_byte >> 1 & 7;

	switch (cmd) {
	case 0:
		output_bits_msb_first(nds, rtc.stat1);
		rtc.stat1 &= ~0xD;
		break;
	case 1:
		output_bits_msb_first(nds, rtc.stat2);
		break;
	case 2:
		output_bits_lsb_first(nds, rtc.year);
		output_bits_lsb_first(nds, rtc.month);
		output_bits_lsb_first(nds, rtc.day);
		output_bits_lsb_first(nds, rtc.weekday);
		[[fallthrough]];
	case 3:
	{
		output_bits_lsb_first(nds, rtc.hour);
		output_bits_lsb_first(nds, rtc.minute);
		output_bits_lsb_first(nds, rtc.second);
		break;
	}
	case 4:
		int1_read(nds);
		break;
	case 5:
		alarm_read(nds, 1);
		break;
	case 6:
		output_bits_msb_first(nds, rtc.clock_correction);
		break;
	case 7:
		output_bits_msb_first(nds, rtc.free_reg);
		break;
	default:
		LOG("[rtc] unhandled read command: %d\n", cmd);
	}
}

static void
execute_write_command(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	u8 cmd = rtc.cmd_byte >> 1 & 7;

	switch (cmd) {
	case 0:
		stat1_write(nds, rtc.cmd_params[0]);
		break;
	case 1:
		stat2_write(nds, rtc.cmd_params[0]);
		break;
	case 2:
		year_write(nds, reverse_bits(rtc.cmd_params[0]));
		month_write(nds, reverse_bits(rtc.cmd_params[1]));
		day_write(nds, reverse_bits(rtc.cmd_params[2]));
		weekday_write(nds, reverse_bits(rtc.cmd_params[3]));
		hour_write(nds, reverse_bits(rtc.cmd_params[4]));
		minute_write(nds, reverse_bits(rtc.cmd_params[5]));
		second_write(nds, reverse_bits(rtc.cmd_params[6]));
		break;
	case 3:
		hour_write(nds, reverse_bits(rtc.cmd_params[0]));
		minute_write(nds, reverse_bits(rtc.cmd_params[1]));
		second_write(nds, reverse_bits(rtc.cmd_params[2]));
		break;
	case 4:
		int1_write(nds);
		break;
	case 5:
		alarm_write(nds, 1, reverse_bits(rtc.cmd_params[0]),
				reverse_bits(rtc.cmd_params[1]),
				reverse_bits(rtc.cmd_params[2]));
		break;
	case 6:
		rtc.clock_correction = rtc.cmd_params[0];
		break;
	case 7:
		rtc.free_reg = rtc.cmd_params[0];
		break;
	default:
		LOG("[rtc] unhandled write command: %d\n", cmd);
	}
}

static void
stat1_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;

	if (value & BIT(7)) {
		rtc_reset(nds);
	}

	rtc.stat1 = (rtc.stat1 & ~0x70) | (value & 0x70);
}

static void
stat2_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	rtc.stat2 = value;
	int old_interrupt_mode = rtc.interrupt_mode;

	switch (value & 0xF0) {
	case 0x00:
		rtc.interrupt_mode = INT_NONE;
		break;
	case 0x80:
	case 0xA0:
		rtc.interrupt_mode = INT_USERFREQ;
		break;
	case 0x40:
	case 0x60:
		rtc.interrupt_mode = INT_MINUTE_EDGE;
		break;
	case 0xC0:
		rtc.interrupt_mode = INT_MINUTE_PERIOD1;
		break;
	case 0x20:
		rtc.interrupt_mode = INT_ALARM1;
		break;
	case 0xE0:
		rtc.interrupt_mode = INT_MINUTE_PERIOD2;
		break;
	default:
		rtc.interrupt_mode = INT_32768;
	}

	if (rtc.interrupt_mode != old_interrupt_mode) {
		rtc.irq_out[0] = false;
	}

	if (!(value & BIT(1))) {
		rtc.irq_out[1] = false;
	}
}

static void
int1_read(nds_ctx *nds)
{
	auto& rtc = nds->rtc;

	switch (rtc.interrupt_mode) {
	case INT_ALARM1:
		alarm_read(nds, 0);
		break;
	case INT_USERFREQ:
		output_bits_lsb_first(nds, rtc.intreg[0][2]);
		break;
	default:
		LOG("[rtc] invalid int1 read, stat2: %02X\n", rtc.stat2);
	}
}

static void
int1_write(nds_ctx *nds)
{
	auto& rtc = nds->rtc;

	switch (rtc.interrupt_mode) {
	case INT_ALARM1:
		alarm_write(nds, 0, reverse_bits(rtc.cmd_params[0]),
				reverse_bits(rtc.cmd_params[1]),
				reverse_bits(rtc.cmd_params[2]));
		break;
	case INT_USERFREQ:
		rtc.intreg[0][2] = reverse_bits(rtc.cmd_params[0]);
		break;
	default:
		LOG("[rtc] invalid int1 write, stat2: %02X\n", rtc.stat2);
	}
}

static void
alarm_read(nds_ctx *nds, int id)
{
	auto& rtc = nds->rtc;
	output_bits_lsb_first(nds, rtc.intreg[id][0]);
	output_bits_lsb_first(nds, rtc.intreg[id][1]);
	output_bits_lsb_first(nds, rtc.intreg[id][2]);
}

static void
alarm_write(nds_ctx *, int id, u8 p0, u8 p1, u8 p2)
{
	LOG("[rtc] alarm write %d, %02X %02X %02X\n", id, p0, p1, p2);
}

static void
year_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	int year = from_bcd_checked(value);

	if (0 <= year && year <= 99) {
		rtc.year = value;
	} else {
		rtc.year = 0x00;
	}
}

static void
month_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	value &= 0x1F;
	int month = from_bcd_checked(value);

	if (1 <= month && month <= 12) {
		rtc.month = value;
	} else {
		rtc.month = 0x01;
	}
}

static void
day_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	value &= 0x3F;
	int day = from_bcd_checked(value);

	if (1 <= day && day <= 31) {
		if (day > get_days_in_month(from_bcd(rtc.month),
					  2000 + from_bcd(rtc.year))) {
			rtc.day = 0x01;
			tick_month(nds);
		} else {
			rtc.day = value;
		}
	} else {
		rtc.day = 0x01;
	}
}

static void
weekday_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	rtc.weekday = value & 7;
}

static void
hour_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	value &= 0x7F;
	int hour = from_bcd_checked(value & 0x3F);

	if (rtc.stat1 & BIT(6)) {
		if (0 <= hour && hour <= 23) {
			rtc.hour = value & 0x3F;
			if (hour >= 12) {
				rtc.hour |= BIT(6);
			}
		} else {
			rtc.hour = 0x00;
		}
	} else {
		if (0 <= hour && hour <= 11) {
			rtc.hour = value;
		} else {
			rtc.hour = 0x00;
		}
	}
}

static void
minute_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	value &= 0x7F;
	int minute = from_bcd_checked(value);

	if (0 <= minute && minute <= 59) {
		rtc.minute = value;
	} else {
		rtc.minute = 0;
	}
}

static void
second_write(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;
	value &= 0x7F;
	int second = from_bcd_checked(value);

	if (0 <= second && second <= 59) {
		rtc.second = value;
	} else {
		rtc.second = 0x59;
	}
}

static void
tick_second(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	rtc.second += 0x01;

	if ((rtc.second & 0x0F) == 0x0A) {
		rtc.second = (rtc.second + 0x10) & 0xF0;
	}

	if (rtc.second == 0x60) {
		rtc.second = 0x00;
		tick_minute(nds);
	}
}

static void
tick_minute(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	rtc.minute += 0x01;

	if ((rtc.minute & 0x0F) == 0x0A) {
		rtc.minute = (rtc.minute + 0x10) & 0xF0;
	}

	if (rtc.minute == 0x60) {
		rtc.minute = 0x00;
		tick_hour(nds);
	}
}

static void
tick_hour(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	rtc.hour += 0x01;

	if ((rtc.hour & 0x0F) == 0x0A) {
		rtc.hour = (rtc.hour + 0x10) & 0xF0;
	}

	if (rtc.stat1 & BIT(6)) {
		if ((rtc.hour & 0x3F) == 0x12) {
			rtc.hour = 0x40;
		} else if ((rtc.hour & 0x3F) == 0x24) {
			rtc.hour = 0x00;
			tick_day(nds);
		}
	} else {
		if ((rtc.hour & 0x3F) == 0x12) {
			if (rtc.hour & BIT(6)) {
				rtc.hour = 0x00;
				tick_day(nds);
			} else {
				rtc.hour = 0x40;
			}
		}
	}
}

static void
tick_day(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	rtc.weekday = (rtc.weekday + 1) & 7;
	rtc.day += 0x01;

	if ((rtc.day & 0x0F) == 0x0A) {
		rtc.day = (rtc.day + 0x10) & 0xF0;
	}

	if (from_bcd(rtc.day) > get_days_in_month(from_bcd(rtc.month),
						2000 + from_bcd(rtc.year))) {
		rtc.day = 0x01;
		tick_month(nds);
	}
}

static void
tick_month(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	rtc.month += 0x01;

	if ((rtc.month & 0x0F) == 0x0A) {
		rtc.month = (rtc.month + 0x10) & 0xF0;
	}

	if (rtc.month == 0x13) {
		rtc.month = 0x01;
		rtc_year(nds);
	}
}

static void
rtc_year(nds_ctx *nds)
{
	auto& rtc = nds->rtc;
	rtc.year += 0x01;

	if ((rtc.year & 0x0F) == 0x0A) {
		rtc.year = (rtc.year + 0x10) & 0xF0;
	}

	if (rtc.year == 0xA0) {
		rtc.year = 0x00;
	}
}

static void
rtc_reset(nds_ctx *)
{
	LOG("[rtc] unhandled reset\n");
}

static void
set_irq_out(nds_ctx *nds, int id, bool value)
{
	auto& rtc = nds->rtc;
	bool old_irq_out = rtc.irq_out[0] || rtc.irq_out[1];
	rtc.irq_out[id] = value;
	bool irq_out = rtc.irq_out[0] || rtc.irq_out[1];

	if (!old_irq_out && irq_out && (nds->rcnt & 0xC100) == 0x8100) {
		request_interrupt(nds->cpu[1], 7);
	}
}

static void
output_bits_msb_first(nds_ctx *nds, u8 value)
{
	output_bits_lsb_first(nds, reverse_bits(value));
}

static void
output_bits_lsb_first(nds_ctx *nds, u8 value)
{
	auto& rtc = nds->rtc;

	for (int count = 8; count--;) {
		rtc.output_bits.push(value & 1);
		value >>= 1;
	}
}

static u8
to_bcd(int x)
{
	return x / 10 << 4 | x % 10;
}

static int
from_bcd(u8 x)
{
	return 10 * (x >> 4) + (x & 0xF);
}

static int
from_bcd_checked(u8 x)
{
	int digit0 = x & 0xF;
	int digit1 = x >> 4;

	if (digit0 > 9 || digit1 > 9) {
		return -1;
	}

	return digit1 * 10 + digit0;
}

static u8
reverse_bits(u8 x)
{
	x = (x & 0x0F) << 4 | (x & 0xF0) >> 4;
	x = (x & 0x33) << 2 | (x & 0xCC) >> 2;
	x = (x & 0x55) << 1 | (x & 0xAA) >> 1;
	return x;
}

} // namespace twice
