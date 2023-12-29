#include "nds/mem/bus.h"
#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

enum {
	PSG_INVALID,
	PSG_WAVE,
	PSG_NOISE,
};

static const s32 adpcm_index_table[] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static const s32 adpcm_table[128] = {                                   //
	0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, //
	0x0010, 0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F, //
	0x0022, 0x0025, 0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042, //
	0x0049, 0x0050, 0x0058, 0x0061, 0x006B, 0x0076, 0x0082, 0x008F, //
	0x009D, 0x00AD, 0x00BE, 0x00D1, 0x00E6, 0x00FD, 0x0117, 0x0133, //
	0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE, 0x0220, 0x0256, 0x0292, //
	0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E, 0x0502, 0x0583, //
	0x0610, 0x06AB, 0x0756, 0x0812, 0x08E0, 0x09C3, 0x0ABD, 0x0BD0, //
	0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954, //
	0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B, //
	0x3BB9, 0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462, //
	0x7FFF
};

static const s32 psg_wave_duty_table[8][8] = {
	{ 0, 0, 0, 0, 0, 0, 0, 1 },
	{ 0, 0, 0, 0, 0, 0, 1, 1 },
	{ 0, 0, 0, 0, 0, 1, 1, 1 },
	{ 0, 0, 0, 0, 1, 1, 1, 1 },
	{ 0, 0, 0, 1, 1, 1, 1, 1 },
	{ 0, 0, 1, 1, 1, 1, 1, 1 },
	{ 0, 1, 1, 1, 1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
};

static void mix_audio(nds_ctx *nds, s64 *l_out, s64 *r_out);
static void sample_audio_channels(nds_ctx *nds, s64 *mixer_l, s64 *mixer_r,
		s64 *ch_l, s64 *ch_r);
static void sample_channel(nds_ctx *, int ch_id, s64 *l_out, s64 *r_out);
static void start_channel(nds_ctx *nds, int ch_id);
static void run_channel(nds_ctx *nds, int ch_id, u32 cycles);
static void step_channel(nds_ctx *nds, int ch_id);
static void start_capture_channel(nds_ctx *nds, int ch_id);
static void run_capture_channel(nds_ctx *, int ch_id, u32 cycles, s16 sample);
static void step_capture_channel(nds_ctx *nds, int ch_id, s16 sample);
static void send_audio_samples(nds_ctx *nds, s16 left, s16 right);
static void write_tmr_reload(nds_ctx *nds, int ch_id, u16 value);

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
		ch.start = !(ch.cnt & BIT(31)) && value & BIT(7);
		ch.cnt = (ch.cnt & ~0xFF000000) |
		         ((u32)value << 24 & 0xFF000000);
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
		write_tmr_reload(nds, addr >> 4, value);
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
		ch.start = ~ch.cnt & value & BIT(31);
		ch.cnt = value & 0xFF7F837F;
		return;
	}
	case 0x4:
		ch.sad = value & 0x07FFFFFC;
		return;
	case 0x8:
		write_tmr_reload(nds, addr >> 4, value);
		ch.pnt = value >> 16 & 0xFFFF;
		return;
	case 0xC:
		ch.len = value & 0x3FFFFF;
		return;
	}

	LOG("sound write 32 at %02X\n", addr);
}

void
sound_capture_write_cnt(nds_ctx *nds, int ch_id, u8 value)
{
	auto& ch = nds->sound_cap_ch[ch_id];
	bool start = ~ch.cnt & value & 0x80;

	ch.cnt = value & 0x8F;
	/* TODO: start when ready? */
	if (start) {
		start_capture_channel(nds, ch_id);
	}
}

void
sound_frame_end(nds_ctx *nds)
{
	nds->last_audio_buf_size = nds->audio_buf_idx * sizeof *nds->audio_buf;
	nds->audio_buf_idx = 0;
}

void
event_sample_audio(nds_ctx *nds, intptr_t, timestamp late)
{
	bool mixer_enabled = nds->soundcnt & BIT(15);
	bool amp_enabled = nds->pwr.ctrl & BIT(0);
	bool muted = nds->pwr.ctrl & BIT(1);

	if (amp_enabled && muted) {
		send_audio_samples(nds, 0, 0);
	} else {
		s64 left = 0, right = 0;

		/* 14.31 fixed point */
		if (mixer_enabled) {
			mix_audio(nds, &left, &right);
		}

		if (nds->use_16_bit_audio) {
			left >>= 25;
			right >>= 25;
			left += ((s64)nds->soundbias << 10) - 0x80000;
			right += ((s64)nds->soundbias << 10) - 0x80000;
			left = std::clamp(left, (s64)-0x8000, (s64)0x7FFF);
			right = std::clamp(right, (s64)-0x8000, (s64)0x7FFF);
		} else {
			left >>= 31;
			right >>= 31;
			left += nds->soundbias;
			right += nds->soundbias;
			left = std::clamp(left, (s64)0, (s64)0x3FF);
			right = std::clamp(right, (s64)0, (s64)0x3FF);
			left -= 0x200;
			right -= 0x200;
			left <<= 6;
			right <<= 6;
		}

		if (!amp_enabled) {
			left >>= 6;
			right >>= 6;
		}

		send_audio_samples(nds, left, right);
	}
}

static void
mix_audio(nds_ctx *nds, s64 *l_out, s64 *r_out)
{
	s64 left = 0, right = 0;
	s64 mixer_l = 0, mixer_r = 0;
	s64 ch_l[16]{}, ch_r[16]{};

	sample_audio_channels(nds, &mixer_l, &mixer_r, ch_l, ch_r);

	switch (nds->soundcnt >> 8 & 3) {
	case 0:
		left = mixer_l;
		break;
	case 1:
		left = ch_l[1];
		break;
	case 2:
		left = ch_l[3];
		break;
	case 3:
		left = ch_l[1] + ch_l[3];
	}

	switch (nds->soundcnt >> 10 & 3) {
	case 0:
		right = mixer_r;
		break;
	case 1:
		right = ch_r[1];
		break;
	case 2:
		right = ch_r[3];
		break;
	case 3:
		right = ch_r[1] + ch_r[3];
	}

	s64 vol = (nds->soundcnt & 0x7F) != 0x7F ? (nds->soundcnt & 0x7F)
	                                         : 0x80;
	*l_out = left * vol;
	*r_out = right * vol;
}

static void
sample_audio_channels(
		nds_ctx *nds, s64 *mixer_l, s64 *mixer_r, s64 *ch_l, s64 *ch_r)
{
	for (int ch_id = 0; ch_id < 16; ch_id++) {
		auto& ch = nds->sound_ch[ch_id];
		bool enabled = ch.cnt & BIT(31);
		bool ch1_to_mixer = !(nds->soundcnt & BIT(12));
		bool ch3_to_mixer = !(nds->soundcnt & BIT(13));

		if (!enabled)
			continue;

		sample_channel(nds, ch_id, &ch_l[ch_id], &ch_r[ch_id]);
		run_channel(nds, ch_id, nds->timer_32k_last_period);

		if (ch_id == 1 && !ch1_to_mixer)
			continue;
		if (ch_id == 3 && !ch3_to_mixer)
			continue;

		*mixer_l += ch_l[ch_id];
		*mixer_r += ch_r[ch_id];
	}

	for (int ch_id = 0; ch_id < 2; ch_id++) {
		auto& ch = nds->sound_cap_ch[ch_id];
		bool enabled = ch.cnt & BIT(7);
		bool sample_from_channel = ch.cnt & BIT(1);

		if (!enabled)
			continue;

		s64 sample;
		if (sample_from_channel) {
			/* TODO: not clipped (capture bugs) */
			sample = ch_l[ch_id << 1] + ch_r[ch_id << 1];
		} else {
			sample = ch_id == 0 ? *mixer_l : *mixer_r;
		}

		sample = std::clamp(
				sample, (s64)-0x8000 << 18, (s64)0x7FFF << 18);
		if (sample >= 0) {
			sample = sample >> 18;
		} else {
			sample = -(-sample >> 18);
		}

		run_capture_channel(nds, ch_id, nds->timer_32k_last_period,
				sample);
	}
}

static void
sample_channel(nds_ctx *nds, int ch_id, s64 *l_out, s64 *r_out)
{
	auto& ch = nds->sound_ch[ch_id];

	if (ch.start) {
		start_channel(nds, ch_id);
	}

	/* sample is in 16.0 fixed point */
	s64 sample = 0;
	switch (ch.cnt >> 29 & 3) {
	case 0:
		sample = (s32)(s8)bus7_read<u8>(nds, ch.addr) << 8;
		break;
	case 1:
		sample = (s16)bus7_read<u16>(nds, ch.addr);
		break;
	case 2:
		sample = ch.adpcm.value;
		break;
	case 3:
		sample = ch.psg.value;
	}

	/* 16.4 fixed point */
	sample <<= 4;
	sample >>= (ch.cnt >> 8 & 3) != 3 ? (ch.cnt >> 8 & 3) : 4;

	/* 16.11 fixed point */
	sample *= (ch.cnt & 0x7F) != 0x7F ? (ch.cnt & 0x7F) : 0x80;

	/* 16.18 fixed point */
	s32 pan = (ch.cnt >> 16 & 0x7F) != 0x7F ? (ch.cnt >> 16 & 0x7F) : 0x80;
	*l_out = sample * (0x80 - pan);
	*r_out = sample * pan;
}

static void
start_channel(nds_ctx *nds, int ch_id)
{
	auto& ch = nds->sound_ch[ch_id];
	ch.tmr = ch.tmr_reload << 2;
	ch.addr = ch.sad;
	ch.start = false;

	switch (ch.cnt >> 29 & 3) {
	case 2:
		ch.adpcm.header = bus7_read<u32>(nds, ch.addr);
		ch.adpcm.value = (s16)ch.adpcm.header;
		ch.adpcm.index = ch.adpcm.header >> 16 & 0x7F;
		ch.adpcm.first = true;
		ch.addr += 4;
		break;
	case 3:
		if (8 <= ch_id && ch_id <= 13) {
			ch.psg.mode = PSG_WAVE;
			ch.psg.wave_pos = 0;
		} else if (14 <= ch_id && ch_id <= 15) {
			ch.psg.mode = PSG_NOISE;
			ch.psg.lfsr = 0x7FFF;
		} else {
			ch.psg.mode = PSG_INVALID;
		}
		ch.psg.value = 0;
	}
}

static void
run_channel(nds_ctx *nds, int ch_id, u32 cycles)
{
	auto& ch = nds->sound_ch[ch_id];

	while (cycles != 0) {
		u32 elapsed = cycles;
		ch.tmr += cycles;

		if (ch.tmr >= (u32)0x10000 << 2) {
			elapsed -= ch.tmr - ((u32)0x10000 << 2);
			step_channel(nds, ch_id);
			ch.tmr = ch.tmr_reload << 2;
		}

		cycles -= elapsed;
	}
}

static void
step_channel(nds_ctx *nds, int ch_id)
{
	auto& ch = nds->sound_ch[ch_id];
	u32 format = ch.cnt >> 29 & 3;

	switch (format) {
	case 0:
		ch.addr += 1;
		break;
	case 1:
		ch.addr += 2;
		break;
	case 2:
	{
		if (ch.adpcm.first && ch.addr == ch.sad + ch.pnt * 4) {
			ch.adpcm.value_s = ch.adpcm.value;
			ch.adpcm.index_s = ch.adpcm.index;
		}

		if (ch.adpcm.first) {
			ch.adpcm.data = bus7_read<u8>(nds, ch.addr);
		} else {
			ch.adpcm.data >>= 4;
			ch.addr += 1;
		}

		s32 diff = adpcm_table[ch.adpcm.index] >> 3;
		if (ch.adpcm.data & 1)
			diff += adpcm_table[ch.adpcm.index] >> 2;
		if (ch.adpcm.data & 2)
			diff += adpcm_table[ch.adpcm.index] >> 1;
		if (ch.adpcm.data & 4)
			diff += adpcm_table[ch.adpcm.index];

		if ((ch.adpcm.data & 8) == 0) {
			ch.adpcm.value = std::min(
					ch.adpcm.value + diff, 0x7FFF);
		} else {
			ch.adpcm.value = std::max(
					ch.adpcm.value - diff, -0x7FFF);
		}

		ch.adpcm.index += adpcm_index_table[ch.adpcm.data & 7];
		ch.adpcm.index = std::clamp(ch.adpcm.index, 0, 88);
		ch.adpcm.first ^= 1;
		break;
	}
	case 3:
	{
		switch (ch.psg.mode) {
		case PSG_WAVE:
		{
			u32 duty = ch.cnt >> 24 & 7;
			u32 idx = ch.psg.wave_pos++ & 7;
			ch.psg.value = psg_wave_duty_table[duty][idx]
			                               ? 0x7FFF
			                               : -0x7FFF;
			break;
		}
		case PSG_NOISE:
		{
			bool carry = ch.psg.lfsr & 1;
			ch.psg.lfsr >>= 1;
			if (carry) {
				ch.psg.lfsr ^= 0x6000;
				ch.psg.value = -0x7FFF;
			} else {
				ch.psg.value = 0x7FFF;
			}
		}
		}
	}
	}

	if (ch.addr == ch.sad + 4 * ch.pnt + 4 * ch.len) {
		switch (ch.cnt >> 27 & 3) {
		case 1:
		case 3:
			ch.addr = ch.sad + 4 * ch.pnt;
			switch (format) {
			case 2:
				ch.adpcm.value = ch.adpcm.value_s;
				ch.adpcm.index = ch.adpcm.index_s;
				ch.adpcm.first = true;
				break;
			case 3:
				switch (ch.psg.mode) {
				case PSG_WAVE:
					ch.psg.wave_pos = 0;
					break;
				case PSG_NOISE:
					ch.psg.lfsr = 0x7FFF;
				}
			}
			break;
		case 2:
			ch.cnt &= ~BIT(31);
		}
	}
}

static void
start_capture_channel(nds_ctx *nds, int ch_id)
{
	auto& ch = nds->sound_cap_ch[ch_id];
	ch.tmr = ch.tmr_reload << 2;
	ch.addr = ch.dad;
}

static void
run_capture_channel(nds_ctx *nds, int ch_id, u32 cycles, s16 value)
{
	auto& ch = nds->sound_cap_ch[ch_id];

	while (cycles != 0) {
		u32 elapsed = cycles;
		ch.tmr += cycles;

		if (ch.tmr >= (u32)0x10000 << 2) {
			elapsed -= ch.tmr - ((u32)0x10000 << 2);
			step_capture_channel(nds, ch_id, value);
			ch.tmr = ch.tmr_reload << 2;
		}

		cycles -= elapsed;
	}
}

static void
step_capture_channel(nds_ctx *nds, int ch_id, s16 value)
{
	auto& ch = nds->sound_cap_ch[ch_id];
	bool pcm8 = ch.cnt & BIT(3);
	bool one_shot = ch.cnt & BIT(2);

	if (pcm8) {
		bus7_write<u8>(nds, ch.addr, value >> 8);
		ch.addr += 1;
	} else {
		bus7_write<u16>(nds, ch.addr, value);
		ch.addr += 2;
	}

	u32 len_bytes = ch.len * 4;
	if (len_bytes == 0) {
		len_bytes = 4;
	}

	if (ch.addr == ch.dad + len_bytes) {
		if (one_shot) {
			ch.cnt &= ~BIT(7);
		} else {
			ch.addr = ch.dad;
		}
	}
}

static void
send_audio_samples(nds_ctx *nds, s16 left, s16 right)
{
	nds->audio_buf[nds->audio_buf_idx++] = left;
	nds->audio_buf[nds->audio_buf_idx++] = right;
}

static void
write_tmr_reload(nds_ctx *nds, int ch_id, u16 value)
{
	nds->sound_ch[ch_id].tmr_reload = value;
	if (ch_id == 1 || ch_id == 3) {
		nds->sound_cap_ch[ch_id >> 1].tmr_reload = value;
	}
}

} // namespace twice
