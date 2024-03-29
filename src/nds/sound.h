#ifndef TWICE_SOUND_H
#define TWICE_SOUND_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct sound_fifo {
	u32 buf[8]{};
	u32 addr{};
	u32 end_addr{};
	u32 read_idx{};
	u32 write_idx{};
	u32 size{};
};

struct sound_channel {
	u32 cnt{};
	u32 sad{};
	u32 tmr_reload{};
	u32 pnt{};
	u32 len{};
	u32 tmr{};
	s32 count{};
	s32 loop_start{};
	s32 loop_end{};
	s32 sample{};
	s32 prev_sample{};
	sound_fifo fifo;
	bool start{};

	struct adpcm_state {
		u32 header;
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
};

struct sound_capture_channel {
	u8 cnt;
	u32 dad;
	u32 len;
	u32 tmr_reload{};
	u32 tmr{};
	sound_fifo fifo;
	bool start{};
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
