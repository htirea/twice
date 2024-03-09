#ifndef TWICE_SOUND_H
#define TWICE_SOUND_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct sound_channel {
	u32 cnt{};
	u32 sad{};
	u32 tmr_reload{};
	u32 pnt{};
	u32 len{};
	bool start{};
	u32 tmr{};
	u32 addr{};

	struct adpcm_state {
		u32 header;
		bool first;
		s32 value;
		s32 index;
		s32 value_s;
		s32 index_s;
		u8 data;
	};

	struct psg_state {
		int mode;
		u32 wave_pos;
		u16 lfsr;
		s32 value;
	};

	union {
		adpcm_state adpcm;
		psg_state psg;
	};

	s32 sample{};
	s32 prev_sample{};
};

struct sound_capture_channel {
	u8 cnt;
	u32 dad;
	u32 len;
	u32 addr{};
	u32 tmr_reload{};
	u32 tmr{};
};

u8 sound_read8(nds_ctx *nds, u8 addr);
u16 sound_read16(nds_ctx *nds, u8 addr);
u32 sound_read32(nds_ctx *nds, u8 addr);
void sound_write8(nds_ctx *nds, u8 addr, u8 value);
void sound_write16(nds_ctx *nds, u8 addr, u16 value);
void sound_write32(nds_ctx *nds, u8 addr, u32 value);
void sound_capture_write_cnt(nds_ctx *nds, int ch_id, u8 value);
void sound_frame_end(nds_ctx *nds);
void sample_audio(nds_ctx *nds);

} // namespace twice

#endif
