#ifndef TWICE_UTIL_H
#define TWICE_UTIL_H

#include "common/types.h"

namespace twice {

template <typename T>
T
readarr(const u8 *arr, u32 offset)
{
	T x;
	std::memcpy(&x, arr + offset, sizeof(T));
	return x;
}

template <typename T>
void
writearr(u8 *arr, u32 offset, T data)
{
	std::memcpy(arr + offset, &data, sizeof(T));
}

template <typename T>
T
readarr_checked(const u8 *arr, u32 offset, size_t size, T default_value)
{
	if (offset + sizeof(T) > size) {
		return default_value;
	}

	return readarr<T>(arr, offset);
}

template <typename T>
void
writearr_checked(u8 *arr, u32 offset, T data, size_t size)
{
	if (offset + sizeof(T) > size) {
		return;
	}

	writearr(arr, offset, data);
}

constexpr u64
BIT(int x)
{
	return (u64)1 << x;
}

constexpr u64
MASK(int x)
{
	if (x == 64) {
		return -1;
	} else {
		return ((u64)1 << x) - 1;
	}
}

template <int n>
constexpr s32
SEXT(u32 x)
{
	static_assert(n != 0 && n <= 32);

	return (s32)(x << (32 - n)) >> (32 - n);
}

template <int n>
constexpr s64
SEXTL(u64 x)
{
	static_assert(n != 0 && n <= 64);

	return (s64)(x << (64 - n)) >> (64 - n);
}

inline s32
ilerp(s32 y0, s32 y1, s32 x0, s32 x1, s32 x)
{
	s64 numer = y0 * ((s64)x1 - x) + y1 * ((s64)x - x0);
	s64 denom = (s64)x1 - x0;

	return denom == 0 ? y0 : numer / denom;
}

} // namespace twice

#endif
