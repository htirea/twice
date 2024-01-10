#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

static void output_12_bit_value(touchscreen *ts, u16 value);
static void output_8_bit_value(touchscreen *ts, u8 value);

void
touchscreen_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& ts = nds->ts;
	bool start = value & BIT(7);

	if (!ts.output_bytes.empty()) {
		nds->spidata_r = ts.output_bytes.front();
		ts.output_bytes.pop();
	}

	if (start) {
		u8 channel = value >> 4 & 7;
		switch (channel) {
		case 0: /* temp0 */
			output_12_bit_value(&ts, 0);
			break;
		case 1: /* y pos */
			output_12_bit_value(&ts, ts.down ? ts.raw_y : 0xFFF);
			break;
		case 2: /* battery voltage */
			output_12_bit_value(&ts, 0);
			break;
		case 3: /* z1 pos */
		case 4: /* z2 pos */
			output_12_bit_value(&ts, 0);
			LOGV("[ts] ch %d z pos\n", channel);
			break;
		case 5: /* x pos */
			output_12_bit_value(&ts, ts.down ? ts.raw_x : 0);
			break;
		case 6: /* TODO: microphone */
			output_8_bit_value(&ts, 0);
			break;
		case 7: /* temp1 */
			output_12_bit_value(&ts, 142);
			break;
		}
	}

	ts.cs_active = keep_active;
}

void
touchscreen_spi_reset(nds_ctx *nds)
{
	auto& ts = nds->ts;
	ts.cs_active = false;
}

void
touchscreen_tick_32k(nds_ctx *nds)
{
	auto& ts = nds->ts;

	if (ts.release_countdown > 0) {
		ts.release_countdown--;
		if (ts.release_countdown == 0) {
			nds_set_touchscreen_state(
					nds, 0, 0, false, false, false);
		}
	}
}

void
nds_set_touchscreen_state(nds_ctx *nds, int x, int y, bool down, bool quicktap,
		bool moved)
{
	auto& ts = nds->ts;
	ts.down = down;

	if (moved && !down)
		return;

	if (down) {
		ts.raw_x = std::clamp(x << 4, 1 << 4, 255 << 4);
		ts.raw_y = std::clamp(y << 4, 1 << 4, 191 << 4);
		nds->extkeyin &= ~BIT(6);
	} else {
		nds->extkeyin |= BIT(6);
	}

	if (quicktap) {
		ts.release_countdown = 512;
	}
}

static void
output_12_bit_value(touchscreen *ts, u16 value)
{
	ts->output_bytes.push(value >> 5);
	ts->output_bytes.push((value & 0x1F) << 3);
}

static void
output_8_bit_value(touchscreen *ts, u8 value)
{
	ts->output_bytes.push(value >> 1);
	ts->output_bytes.push((value & 1) << 7);
}

} // namespace twice
