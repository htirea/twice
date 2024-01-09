#ifndef LIBTWICE_NDS_DEFS_H
#define LIBTWICE_NDS_DEFS_H

#include <cstddef>

namespace twice {

constexpr std::size_t NDS_FB_W = 256;
constexpr std::size_t NDS_FB_H = 384;
constexpr std::size_t NDS_FB_SZ = NDS_FB_W * NDS_FB_H;
constexpr std::size_t NDS_FB_SZ_BYTES = NDS_FB_SZ * 4;
constexpr double NDS_FB_ASPECT_RATIO = (double)NDS_FB_W / NDS_FB_H;
constexpr double NDS_FB_ASPECT_RATIO_RECIP = (double)NDS_FB_H / NDS_FB_W;

constexpr std::size_t NDS_SCREEN_W = 256;
constexpr std::size_t NDS_SCREEN_H = 192;
constexpr std::size_t NDS_SCREEN_SZ = NDS_SCREEN_W * NDS_SCREEN_H;

constexpr long NDS_ARM9_CLK_RATE = 67027964;
constexpr long NDS_ARM7_CLK_RATE = 33513982;
constexpr double NDS_FRAME_RATE = NDS_ARM7_CLK_RATE / 560190.0;
constexpr long NDS_AUDIO_SAMPLE_RATE = 32768;
constexpr double NDS_AVG_SAMPLES_PER_FRAME =
		NDS_AUDIO_SAMPLE_RATE / NDS_FRAME_RATE;

} // namespace twice

#endif
