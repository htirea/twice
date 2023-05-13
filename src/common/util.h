#ifndef TWICE_UTIL_H
#define TWICE_UTIL_H

#include "common/types.h"

template <typename T>
T
readarr(u8 *arr, u32 offset)
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

#endif
