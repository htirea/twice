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

#define IO_READ8_COMMON(cpuid_)                                               \
	fprintf(stderr, "nds %d io read 8 at %08X\n", (cpuid_), addr);        \
	return 0;                                                             \
	case 0x4000208:                                                       \
		return nds->cpu[(cpuid_)]->IME;                               \
	case 0x4000210:                                                       \
		IO_READ8_FROM_32(0x4000210, nds->cpu[(cpuid_)]->IE);          \
	case 0x4000214:                                                       \
		IO_READ8_FROM_32(0x4000214, nds->cpu[(cpuid_)]->IF)

#define IO_READ16_COMMON(cpuid_)                                              \
	fprintf(stderr, "nds %d io read 16 at %08X\n", (cpuid_), addr);       \
	return 0;                                                             \
	case 0x4000004:                                                       \
		return nds->dispstat[(cpuid_)];                               \
	case 0x4000130:                                                       \
		return nds->keyinput;                                         \
	case 0x4000180:                                                       \
		return nds->ipcsync[(cpuid_)];                                \
	case 0x4000184:                                                       \
		return nds->ipcfifo[(cpuid_)].cnt;                            \
	case 0x4000208:                                                       \
		return nds->cpu[(cpuid_)]->IME;                               \
	case 0x4000210:                                                       \
		IO_READ16_FROM_32(0x4000210, nds->cpu[(cpuid_)]->IE);         \
	case 0x4000214:                                                       \
		IO_READ16_FROM_32(0x4000214, nds->cpu[(cpuid_)]->IF)

#define IO_READ32_COMMON(cpuid_)                                              \
	fprintf(stderr, "nds %d io read 32 at %08X\n", (cpuid_), addr);       \
	return 0;                                                             \
	case 0x4000208:                                                       \
		return nds->cpu[(cpuid_)]->IME;                               \
	case 0x4000210:                                                       \
		return nds->cpu[(cpuid_)]->IE;                                \
	case 0x4000214:                                                       \
		return nds->cpu[(cpuid_)]->IF;                                \
	case 0x4100000:                                                       \
		return ipc_fifo_recv(nds, (cpuid_))

#define IO_WRITE8_COMMON(cpuid_)                                              \
	fprintf(stderr, "nds %d io write 8 to %08X\n", (cpuid_), addr);       \
	break;                                                                \
	case 0x4000208:                                                       \
		nds->cpu[(cpuid_)]->IME = value & 1;                          \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		break

#define IO_WRITE16_COMMON(cpuid_)                                             \
	fprintf(stderr, "nds %d io write 16 to %08X\n", (cpuid_), addr);      \
	break;                                                                \
	case 0x4000180:                                                       \
		nds->ipcsync[(cpuid_) ^ 1] &= ~0xF;                           \
		nds->ipcsync[(cpuid_) ^ 1] |= value >> 8 & 0xF;               \
		nds->ipcsync[(cpuid_)] &= ~0x4F00;                            \
		nds->ipcsync[(cpuid_)] |= value & 0x4F00;                     \
                                                                              \
		if ((value & BIT(13)) &&                                      \
				(nds->ipcsync[(cpuid_) ^ 1] & BIT(14))) {     \
			nds->cpu[(cpuid_) ^ 1]->request_interrupt(16);        \
		}                                                             \
		break;                                                        \
	case 0x4000184:                                                       \
		ipc_fifo_cnt_write(nds, (cpuid_), value);                     \
		break;                                                        \
	case 0x4000208:                                                       \
		nds->cpu[(cpuid_)]->IME = value & 1;                          \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		break

#define IO_WRITE32_COMMON(cpuid_)                                             \
	fprintf(stderr, "nds %d io write 32 to %08X\n", (cpuid_), addr);      \
	break;                                                                \
	case 0x4000188:                                                       \
		ipc_fifo_send(nds, (cpuid_), value);                          \
		break;                                                        \
	case 0x4000208:                                                       \
		nds->cpu[(cpuid_)]->IME = value & 1;                          \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		break;                                                        \
	case 0x4000210:                                                       \
		nds->cpu[(cpuid_)]->IE = value;                               \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		break;                                                        \
	case 0x4000214:                                                       \
		nds->cpu[(cpuid_)]->IF &= ~value;                             \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		break

#endif
