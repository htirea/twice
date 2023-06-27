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
	case 0x4000208:                                                       \
		return nds->cpu[(cpuid_)]->IME;                               \
	case 0x4000210:                                                       \
		IO_READ8_FROM_32(0x4000210, nds->cpu[(cpuid_)]->IE);          \
	case 0x4000214:                                                       \
		IO_READ8_FROM_32(0x4000214, nds->cpu[(cpuid_)]->IF);          \
	case 0x4000300:                                                       \
		return nds->postflg[cpuid_]

#define IO_READ16_COMMON(cpuid_)                                              \
	case 0x4000004:                                                       \
		return nds->dispstat[(cpuid_)];                               \
	case 0x4000006:                                                       \
		return nds->vcount;                                           \
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
	case 0x40000B0:                                                       \
		return nds->dma_sad[cpuid_][0];                               \
	case 0x40000B4:                                                       \
		return nds->dma_dad[cpuid_][0];                               \
	case 0x40000B8:                                                       \
		return (u32)nds->dmacnt_h[cpuid_][0] << 16 |                  \
				nds->dmacnt_l[cpuid_][0];                     \
	case 0x40000BC:                                                       \
		return nds->dma_sad[cpuid_][1];                               \
	case 0x40000C0:                                                       \
		return nds->dma_dad[cpuid_][1];                               \
	case 0x40000C4:                                                       \
		return (u32)nds->dmacnt_h[cpuid_][1] << 16 |                  \
				nds->dmacnt_l[cpuid_][1];                     \
	case 0x40000C8:                                                       \
		return nds->dma_sad[cpuid_][2];                               \
	case 0x40000CC:                                                       \
		return nds->dma_dad[cpuid_][2];                               \
	case 0x40000D0:                                                       \
		return (u32)nds->dmacnt_h[cpuid_][2] << 16 |                  \
				nds->dmacnt_l[cpuid_][2];                     \
	case 0x40000D4:                                                       \
		return nds->dma_sad[cpuid_][3];                               \
	case 0x40000D8:                                                       \
		return nds->dma_dad[cpuid_][3];                               \
	case 0x40000DC:                                                       \
		return (u32)nds->dmacnt_h[cpuid_][3] << 16 |                  \
				nds->dmacnt_l[cpuid_][3];                     \
	case 0x4000180:                                                       \
		return nds->ipcsync[(cpuid_)];                                \
	case 0x4000208:                                                       \
		return nds->cpu[(cpuid_)]->IME;                               \
	case 0x4000210:                                                       \
		return nds->cpu[(cpuid_)]->IE;                                \
	case 0x4000214:                                                       \
		return nds->cpu[(cpuid_)]->IF;                                \
	case 0x4100000:                                                       \
		return ipc_fifo_recv(nds, (cpuid_))

#define IO_WRITE8_COMMON(cpuid_)                                              \
	case 0x4000208:                                                       \
		nds->cpu[(cpuid_)]->IME = value & 1;                          \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		return

#define IO_WRITE16_COMMON(cpuid_)                                             \
	case 0x4000004:                                                       \
		nds->dispstat[cpuid_] &= 0x47;                                \
		nds->dispstat[cpuid_] |= value & ~0x47;                       \
		return;                                                       \
	case 0x4000180:                                                       \
		ipcsync_write(nds, (cpuid_), value);                          \
		return;                                                       \
	case 0x4000184:                                                       \
		ipc_fifo_cnt_write(nds, (cpuid_), value);                     \
		return;                                                       \
	case 0x4000208:                                                       \
		nds->cpu[(cpuid_)]->IME = value & 1;                          \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		return

#define IO_WRITE32_COMMON(cpuid_)                                             \
	case 0x4000004:                                                       \
		nds->dispstat[cpuid_] &= 0x47;                                \
		nds->dispstat[cpuid_] |= value & ~0x47;                       \
		return;                                                       \
	case 0x40000B0:                                                       \
		nds->dma_sad[cpuid_][0] = value;                              \
		return;                                                       \
	case 0x40000B4:                                                       \
		nds->dma_dad[cpuid_][0] = value;                              \
		return;                                                       \
	case 0x40000B8:                                                       \
		nds->dmacnt_l[cpuid_][0] = value;                             \
		dmacnt_h_write(nds, (cpuid_), 0, value >> 16);                \
		return;                                                       \
	case 0x40000BC:                                                       \
		nds->dma_sad[cpuid_][1] = value;                              \
		return;                                                       \
	case 0x40000C0:                                                       \
		nds->dma_dad[cpuid_][1] = value;                              \
		return;                                                       \
	case 0x40000C4:                                                       \
		nds->dmacnt_l[cpuid_][1] = value;                             \
		dmacnt_h_write(nds, (cpuid_), 1, value >> 16);                \
		return;                                                       \
	case 0x40000C8:                                                       \
		nds->dma_sad[cpuid_][2] = value;                              \
		return;                                                       \
	case 0x40000CC:                                                       \
		nds->dma_dad[cpuid_][2] = value;                              \
		return;                                                       \
	case 0x40000D0:                                                       \
		nds->dmacnt_l[cpuid_][2] = value;                             \
		dmacnt_h_write(nds, (cpuid_), 2, value >> 16);                \
		return;                                                       \
	case 0x40000D4:                                                       \
		nds->dma_sad[cpuid_][3] = value;                              \
		return;                                                       \
	case 0x40000D8:                                                       \
		nds->dma_dad[cpuid_][3] = value;                              \
		return;                                                       \
	case 0x40000DC:                                                       \
		nds->dmacnt_l[cpuid_][3] = value;                             \
		dmacnt_h_write(nds, (cpuid_), 3, value >> 16);                \
		return;                                                       \
	case 0x4000180:                                                       \
		ipcsync_write(nds, (cpuid_), value);                          \
		return;                                                       \
	case 0x4000188:                                                       \
		ipc_fifo_send(nds, (cpuid_), value);                          \
		return;                                                       \
	case 0x4000208:                                                       \
		nds->cpu[(cpuid_)]->IME = value & 1;                          \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		return;                                                       \
	case 0x4000210:                                                       \
		nds->cpu[(cpuid_)]->IE = value;                               \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		return;                                                       \
	case 0x4000214:                                                       \
		nds->cpu[(cpuid_)]->IF &= ~value;                             \
		nds->cpu[(cpuid_)]->check_interrupt();                        \
		return

namespace twice {

void ipcsync_write(nds_ctx *nds, int cpuid, u16 value);
}

#endif
