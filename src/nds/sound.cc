#include "nds/mem/bus.h"
#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

static s32
sample_channel(nds_ctx *nds, int ch_id)
{
	auto& ch = nds->sound_ch[ch_id];

	switch (ch.cnt >> 29 & 3) {
	case 0:
		return (s32)(s8)bus7_read<u8>(nds, ch.address) << 8;
	case 1:
		return (s16)bus7_read<u16>(nds, ch.address);
	case 2:
		return ch.adpcm_value;
	case 3:
		return 0;
	}

	return 0;
}

static const s32 adpcm_index_table[] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static const s32 adpcm_table[128] = { 0x0007, 0x0008, 0x0009, 0x000A, 0x000B,
	0x000C, 0x000D, 0x000E, 0x0010, 0x0011, 0x0013, 0x0015, 0x0017, 0x0019,
	0x001C, 0x001F, 0x0022, 0x0025, 0x0029, 0x002D, 0x0032, 0x0037, 0x003C,
	0x0042, 0x0049, 0x0050, 0x0058, 0x0061, 0x006B, 0x0076, 0x0082, 0x008F,
	0x009D, 0x00AD, 0x00BE, 0x00D1, 0x00E6, 0x00FD, 0x0117, 0x0133, 0x0151,
	0x0173, 0x0198, 0x01C1, 0x01EE, 0x0220, 0x0256, 0x0292, 0x02D4, 0x031C,
	0x036C, 0x03C3, 0x0424, 0x048E, 0x0502, 0x0583, 0x0610, 0x06AB, 0x0756,
	0x0812, 0x08E0, 0x09C3, 0x0ABD, 0x0BD0, 0x0CFF, 0x0E4C, 0x0FBA, 0x114C,
	0x1307, 0x14EE, 0x1706, 0x1954, 0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA,
	0x2CDF, 0x315B, 0x364B, 0x3BB9, 0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F,
	0x69CE, 0x7462, 0x7FFF };

static void
step_channel(nds_ctx *nds, int ch_id)
{
	auto& ch = nds->sound_ch[ch_id];

	switch (ch.cnt >> 29 & 3) {
	case 0:
		ch.address += 1;
		break;
	case 1:
		ch.address += 2;
		break;
	case 2:
	{
		if (ch.adpcm_first && ch.address == ch.sad + ch.pnt * 4) {
			ch.adpcm_value_s = ch.adpcm_value;
			ch.adpcm_index_s = ch.adpcm_index;
		}

		u8 data = bus7_read<u8>(nds, ch.address);
		if (ch.adpcm_first) {
			data &= 0xF;
		} else {
			data >>= 4;
			ch.address += 1;
		}
		ch.adpcm_first ^= 1;

		s32 diff = adpcm_table[ch.adpcm_index] >> 3;
		if (data & 1) {
			diff += adpcm_table[ch.adpcm_index] >> 2;
		}
		if (data & 2) {
			diff += adpcm_table[ch.adpcm_index] >> 1;
		}
		if (data & 4) {
			diff += adpcm_table[ch.adpcm_index];
		}

		if ((data & 8) == 0) {
			ch.adpcm_value = std::min(
					ch.adpcm_value + diff, 0x7FFF);
		} else {
			ch.adpcm_value = std::max(
					ch.adpcm_value - diff, -0x7FFF);
		}
		ch.adpcm_index += adpcm_index_table[data & 7];
		ch.adpcm_index = std::clamp(ch.adpcm_index, 0, 88);
		break;
	}
	case 3:;
	}

	if (ch.address == ch.sad + 4 * ch.pnt + 4 * ch.len) {
		switch (ch.cnt >> 27 & 3) {
		case 1:
		case 3:
			ch.address = ch.sad + 4 * ch.pnt;
			if ((ch.cnt >> 29 & 3) == 2) {
				ch.adpcm_value = ch.adpcm_value_s;
				ch.adpcm_index = ch.adpcm_index_s;
				ch.adpcm_first = true;
			}
			break;
		case 2:
			ch.cnt &= ~BIT(31);
		}
	}
}

static void
sample_audio_channels(nds_ctx *nds, s32 *left_out, s32 *right_out)
{
	s32 ch1_samples[2]{};
	s32 ch3_samples[2]{};
	s32 mixer_samples[2]{};

	for (int ch_id = 0; ch_id < 16; ch_id++) {
		auto& ch = nds->sound_ch[ch_id];

		if (!(ch.cnt & BIT(31)))
			continue;

		s32 value = sample_channel(nds, ch_id);

		for (u32 cycles = 512; cycles != 0;) {
			ch.tmr += cycles;
			u32 elapsed = cycles;

			if (ch.tmr >= 0x10000) {
				elapsed -= ch.tmr & 0xFFFF;
				step_channel(nds, ch_id);
				ch.tmr = ch.tmr_reload;
			}

			cycles -= elapsed;
		}

		s32 volume_div = ch.cnt >> 8 & 3;
		if (volume_div == 3) {
			volume_div += 1;
		}
		value <<= 4 - volume_div;

		s32 volume_factor = ch.cnt & 0x7F;
		if (volume_factor != 0) {
			volume_factor += 1;
		}
		value *= volume_factor;

		s32 pan_factor = ch.cnt >> 16 & 0x7F;
		if (pan_factor != 0) {
			pan_factor += 1;
		}
		s32 left_val = (s64)value * (128 - pan_factor) >> 10;
		s32 right_val = (s64)value * pan_factor >> 10;

		if (ch_id == 1) {
			ch1_samples[0] = left_val;
			ch1_samples[1] = right_val;
			if (nds->soundcnt & BIT(12))
				continue;
		} else if (ch_id == 3) {
			ch3_samples[0] = left_val;
			ch3_samples[1] = right_val;
			if (nds->soundcnt & BIT(13))
				continue;
		}

		mixer_samples[0] += left_val;
		mixer_samples[1] += right_val;
	}

	s32 left = 0;
	s32 right = 0;
	switch (nds->soundcnt >> 8 & 3) {
	case 0:
		left = mixer_samples[0];
		break;
	case 1:
		left = ch1_samples[0];
		break;
	case 2:
		left = ch3_samples[0];
		break;
	case 3:
		left = ch1_samples[0] + ch3_samples[0];
	}
	switch (nds->soundcnt >> 10 & 3) {
	case 0:
		right = mixer_samples[1];
		break;
	case 1:
		right = ch1_samples[1];
		break;
	case 2:
		right = ch3_samples[1];
		break;
	case 3:
		right = ch1_samples[1] + ch3_samples[1];
	}

	s32 master_vol = nds->soundcnt & 0x7F;
	left = (s64)left * master_vol >> 21;
	right = (s64)right * master_vol >> 21;

	*left_out = left;
	*right_out = right;
}

void
event_sample_audio(nds_ctx *nds)
{
	s32 left = 0;
	s32 right = 0;

	if (nds->soundcnt & BIT(15)) {
		sample_audio_channels(nds, &left, &right);
	}

	left += nds->soundbias;
	right += nds->soundbias;
	left = std::clamp(left, 0, 0x3FF);
	right = std::clamp(right, 0, 0x3FF);

	nds->audio_buf[nds->audio_buf_idx++] = (left - 0x200) << 6;
	nds->audio_buf[nds->audio_buf_idx++] = (right - 0x200) << 6;

	reschedule_nds_event_after(nds, event_scheduler::SAMPLE_AUDIO, 1024);
}

static void
start_channel(nds_ctx *nds, int ch_id)
{
	auto& ch = nds->sound_ch[ch_id];

	ch.tmr = ch.tmr_reload;
	ch.address = ch.sad;

	switch (ch.cnt >> 29 & 3) {
	case 2:
		ch.adpcm_header = bus7_read<u32>(nds, ch.address);
		ch.adpcm_value = (s16)ch.adpcm_header;
		ch.adpcm_index = ch.adpcm_header >> 16 & 0x7F;
		ch.adpcm_first = true;
		ch.address += 4;
		break;
	case 3:
		LOG("unimplemented PSG\n");
	}
}

u8
sound_read8(nds_ctx *nds, u8 addr)
{
	auto& ch = nds->sound_ch[addr >> 4];
	u8 offset = addr & 0xF;

	switch (offset) {
	case 0:
	case 1:
	case 2:
	case 3:
		return ch.cnt >> 8 * offset;
	}

	LOG("sound read 8 at %02X\n", addr);
	return 0;
}

u16
sound_read16(nds_ctx *nds, u8 addr)
{
	auto& ch = nds->sound_ch[addr >> 4];
	u8 offset = addr & 0xF;

	switch (offset) {
	case 0:
	case 2:
		return ch.cnt >> 8 * offset;
	}

	LOG("sound read 16 at %02X\n", addr);
	return 0;
}

u32
sound_read32(nds_ctx *nds, u8 addr)
{
	auto& ch = nds->sound_ch[addr >> 4];
	u8 offset = addr & 0xF;

	switch (offset) {
	case 0:
		return ch.cnt;
	}

	LOG("sound read 32 at %02X\n", addr);
	return 0;
}

void
sound_write8(nds_ctx *nds, u8 addr, u8 value)
{
	auto& ch = nds->sound_ch[addr >> 4];
	u8 offset = addr & 0xF;

	switch (offset) {
	case 0:
		ch.cnt = (ch.cnt & ~0x7F) | (value & 0x7F);
		return;
	case 1:
		ch.cnt = (ch.cnt & ~0x8300) | ((u32)value << 8 & 0x8300);
		return;
	case 2:
		ch.cnt = (ch.cnt & ~0x7F0000) | ((u32)value << 16 & 0x7F0000);
		return;
	case 3:
	{
		bool start = !(ch.cnt & BIT(31)) && value & BIT(7);
		ch.cnt = (ch.cnt & ~0xFF000000) |
		         ((u32)value << 24 & 0xFF000000);
		if (start) {
			start_channel(nds, addr >> 4);
		}
		return;
	}
	case 4:
		ch.sad = (ch.sad & ~0xFF) | (value & 0xFC);
		return;
	case 5:
		ch.sad = (ch.sad & ~0xFF00) | ((u32)value << 8 & 0xFF00);
		return;
	case 6:
		ch.sad = (ch.sad & ~0xFF0000) | ((u32)value << 16 & 0xFF0000);
		return;
	case 7:
		ch.sad = (ch.sad & ~0x07000000) |
		         ((u32)value << 24 & 0x07000000);
		return;
	}

	LOG("sound write 8 at %02X\n", addr);
}

void
sound_write16(nds_ctx *nds, u8 addr, u16 value)
{
	auto& ch = nds->sound_ch[addr >> 4];
	u8 offset = addr & 0xF;

	switch (offset) {
	case 0x0:
		ch.cnt = (ch.cnt & ~0x837F) | (value & 0x837F);
		return;
	case 0x8:
		ch.tmr_reload = value;
		return;
	case 0xA:
		ch.pnt = value;
		return;
	}

	LOG("sound write 16 at %02X\n", addr);
}

void
sound_write32(nds_ctx *nds, u8 addr, u32 value)
{
	auto& ch = nds->sound_ch[addr >> 4];
	u8 offset = addr & 0xF;

	switch (offset) {
	case 0x0:
	{
		bool start = ~ch.cnt & value & BIT(31);
		ch.cnt = value & 0xFF7F837F;
		if (start) {
			start_channel(nds, addr >> 4);
		}
		return;
	}
	case 0x4:
		ch.sad = value & 0x07FFFFFC;
		return;
	case 0x8:
		ch.tmr_reload = value & 0xFFFF;
		ch.pnt = value >> 16 & 0xFFFF;
		return;
	case 0xC:
		ch.len = value & 0x3FFFFF;
		return;
	}

	LOG("sound write 32 at %02X\n", addr);
}

} // namespace twice
