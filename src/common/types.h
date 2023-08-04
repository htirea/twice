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
#include <vector>

namespace twice {

using timestamp = u64;
using stimestamp = s64;

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
