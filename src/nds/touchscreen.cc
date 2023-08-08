#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

static void
output_12_bit_value(touchscreen_controller *ts, u16 value)
{
	ts->output_bytes.push(value >> 5);
	ts->output_bytes.push(value & 0x1F);
}

void
touchscreen_spi_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& ts = nds->touchscreen;

	if (ts.output_bytes.empty()) {
		nds->spidata_r = 0;
	} else {
		nds->spidata_r = ts.output_bytes.front();
		ts.output_bytes.pop();
	}

	if (value & BIT(7)) {
		u8 channel = value >> 4 & 7;
		switch (channel) {
		case 1:
			output_12_bit_value(&ts, ts.down ? ts.raw_y : 0xFFF);
			break;
		case 5:
			output_12_bit_value(&ts, ts.down ? ts.raw_x : 0);
			break;
		default:
			LOG("touchscreen channel %d\n", (int)channel);
		}
	}

	ts.cs_active = keep_active;
}

void
touchscreen_spi_reset(nds_ctx *nds)
{
	auto& ts = nds->touchscreen;
	ts.cs_active = false;
}

void
nds_set_touchscreen_state(nds_ctx *nds, int x, int y, bool down)
{
	auto& ts = nds->touchscreen;
	ts.x = x;
	ts.y = y;
	ts.raw_x = std::clamp(x << 4, 1 << 4, 255 << 4);
	ts.raw_y = std::clamp(y << 4, 1 << 4, 191 << 4);
	ts.down = down;
}

} // namespace twice
