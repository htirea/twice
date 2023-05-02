#ifndef LIBTWICE_UTIL_H
#define LIBTWICE_UTIL_H

#include "libtwice/types.h"

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

#endif
