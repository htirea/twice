#ifndef TWICE_BIT_BYTESWAP_H
#define TWICE_BIT_BYTESWAP_H

#include "common/types.h"

namespace twice {

#if defined __has_builtin
#  if __has_builtin(__builtin_bswap32)
#    define TWICE_USE_BUILTIN_BSWAP32
#  endif
#  if __has_builtin(__builtin_bswap64)
#    define TWICE_USE_BUILTIN_BSWAP64
#  endif
#endif

inline u32
byteswap32(u32 x)
{
#ifdef TWICE_USE_BUILTIN_BSWAP32
	return __builtin_bswap32(x);
#else
	x = (x & 0x0000FFFF) << 16 | (x & 0xFFFF0000) >> 16;
	x = (x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8;
	return x;
#endif
}

inline u64
byteswap64(u64 x)
{
#ifdef TWICE_USE_BUILTIN_BSWAP64
	return __builtin_bswap64(x);
#else
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8 | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
#endif
}

} // namespace twice

#endif
