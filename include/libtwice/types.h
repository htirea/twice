#ifndef TWICE_PUBLIC_TYPES_H
#define TWICE_PUBLIC_TYPES_H

#include <cstddef>

namespace twice {

constexpr std::size_t NDS_FB_W = 256;
constexpr std::size_t NDS_FB_H = 384;
constexpr std::size_t NDS_FB_SZ = NDS_FB_W * NDS_FB_H;
constexpr std::size_t NDS_FB_SZ_BYTES = NDS_FB_SZ * 4;

constexpr std::size_t NDS_SCREEN_W = 256;
constexpr std::size_t NDS_SCREEN_H = 192;
constexpr std::size_t NDS_SCREEN_SZ = NDS_SCREEN_W * NDS_SCREEN_H;

} // namespace twice

#endif
