#ifndef TWICE_IO_H
#define TWICE_IO_H

#include "nds/arm/arm.h"
#include "nds/nds.h"

#define IO_READ8_FROM_32(addr, dest)                                          \
	return (dest);                                                        \
	case (addr) + 1:                                                      \
		return (dest) >> 8;                                           \
	case (addr) + 2:                                                      \
		return (dest) >> 16;                                          \
	case (addr) + 3:                                                      \
		return (dest) >> 24

#define IO_READ16_FROM_32(addr, dest)                                         \
	return (dest);                                                        \
	case (addr) + 2:                                                      \
		return (dest) >> 16

#define IO_READ8_FROM_16(addr, dest)                                          \
	return (dest);                                                        \
	case (addr) + 1:                                                      \
		return (dest) >> 8

#endif
