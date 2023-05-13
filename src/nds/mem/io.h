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

namespace twice {

void wramcnt_write(NDS *nds, u8 value);

template <int cpuid>
u8
io_read8(NDS *nds, u32 addr)
{
	switch (addr) {
	case 0x4000208:
		return nds->cpu[cpuid]->IME;
	case 0x4000210:
		IO_READ8_FROM_32(0x4000210, nds->cpu[cpuid]->IE);
	case 0x4000214:
		IO_READ8_FROM_32(0x4000214, nds->cpu[cpuid]->IF);
	}

	fprintf(stderr, "nds %d io read 8 at %08X\n", cpuid, addr);
	return 0;
}

template <int cpuid>
u16
io_read16(NDS *nds, u32 addr)
{
	switch (addr) {
	case 0x4000004:
		return nds->dispstat[cpuid];
	case 0x4000130:
		return nds->keyinput;
	case 0x4000180:
		return nds->ipcsync[cpuid];
	case 0x4000184:
		return nds->ipcfifo[cpuid].cnt;
	case 0x4000208:
		return nds->cpu[cpuid]->IME;
	case 0x4000210:
		IO_READ16_FROM_32(0x4000210, nds->cpu[cpuid]->IE);
	case 0x4000214:
		IO_READ16_FROM_32(0x4000214, nds->cpu[cpuid]->IF);
	}

	fprintf(stderr, "nds %d io read 16 at %08X\n", cpuid, addr);
	return 0;
}

template <int cpuid>
u32
io_read32(NDS *nds, u32 addr)
{
	switch (addr) {
	case 0x4000208:
		return nds->cpu[cpuid]->IME;
	case 0x4000210:
		return nds->cpu[cpuid]->IE;
	case 0x4000214:
		return nds->cpu[cpuid]->IF;
	case 0x4100000:
		return ipc_fifo_recv(nds, cpuid);
	}

	fprintf(stderr, "nds %d io read 32 at %08X\n", cpuid, addr);
	return 0;
}

template <int cpuid>
void
io_write8(NDS *nds, u32 addr, u8 value)
{
	switch (addr) {
	case 0x4000208:
		nds->cpu[cpuid]->IME = value & 1;
		nds->cpu[cpuid]->check_interrupt();
		break;
	default:
		fprintf(stderr, "nds %d io write 8 to %08X\n", cpuid, addr);
	}
}

template <int cpuid>
void
io_write16(NDS *nds, u32 addr, u16 value)
{
	switch (addr) {
	case 0x4000180:
		nds->ipcsync[cpuid ^ 1] &= ~0xF;
		nds->ipcsync[cpuid ^ 1] |= value >> 8 & 0xF;
		nds->ipcsync[cpuid] &= ~0x4F00;
		nds->ipcsync[cpuid] |= value & 0x4F00;

		if ((value & BIT(13)) && (nds->ipcsync[cpuid ^ 1] & BIT(14))) {
			nds->cpu[cpuid ^ 1]->request_interrupt(16);
		}
		break;
	case 0x4000184:
		ipc_fifo_cnt_write(nds, cpuid, value);
		break;
	case 0x4000208:
		nds->cpu[cpuid]->IME = value & 1;
		nds->cpu[cpuid]->check_interrupt();
		break;
	default:
		fprintf(stderr, "nds %d io write 16 to %08X\n", cpuid, addr);
	}
}

template <int cpuid>
void
io_write32(NDS *nds, u32 addr, u32 value)
{
	switch (addr) {
	case 0x4000188:
		ipc_fifo_send(nds, cpuid, value);
		break;
	case 0x4000208:
		nds->cpu[cpuid]->IME = value & 1;
		nds->cpu[cpuid]->check_interrupt();
		break;
	case 0x4000210:
		nds->cpu[cpuid]->IE = value;
		nds->cpu[cpuid]->check_interrupt();
		break;
	case 0x4000214:
		nds->cpu[cpuid]->IF &= ~value;
		nds->cpu[cpuid]->check_interrupt();
		break;
	default:
		fprintf(stderr, "nds %d io write 32 to %08X\n", cpuid, addr);
	}
}

template <typename T, int cpuid>
T
io_read(NDS *nds, u32 addr)
{
	if constexpr (sizeof(T) == 1) {
		return io_read8<cpuid>(nds, addr);
	} else if constexpr (sizeof(T) == 2) {
		return io_read16<cpuid>(nds, addr);
	} else {
		return io_read32<cpuid>(nds, addr);
	}
}

template <typename T, int cpuid>
void
io_write(NDS *nds, u32 addr, T value)
{
	if constexpr (sizeof(T) == 1) {
		io_write8<cpuid>(nds, addr, value);
	} else if constexpr (sizeof(T) == 2) {
		io_write16<cpuid>(nds, addr, value);
	} else {
		io_write32<cpuid>(nds, addr, value);
	}
}

} // namespace twice

#endif
