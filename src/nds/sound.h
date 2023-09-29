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

	u32 tmr{};
	u32 address{};
	u32 adpcm_header{};
	bool adpcm_first{};
	s32 adpcm_value{};
	s32 adpcm_index{};
	s32 adpcm_value_s{};
	s32 adpcm_index_s{};
};

struct sound_capture_channel {
	u8 cnt;
	u32 dad;
	u32 len;

	u32 address{};
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

void event_sample_audio(nds_ctx *nds);
void extend_audio_samples(nds_ctx *nds);

} // namespace twice

#endif
