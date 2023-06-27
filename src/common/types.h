#ifndef TWICE_TYPES_H
#define TWICE_TYPES_H

#include "libtwice/types.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

namespace twice {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using size_t = std::size_t;

constexpr size_t operator""_KiB(unsigned long long v)
{
	return v << 10;
}

constexpr size_t operator""_MiB(unsigned long long v)
{
	return v << 20;
}

constexpr size_t operator""_GiB(unsigned long long v)
{
	return v << 30;
}

} // namespace twice

#endif
