#ifndef TWICE_IPC_H
#define TWICE_IPC_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct NDS;

struct IpcFifo {
	std::array<u32, 16> fifo{};
	u32 read{};
	u32 write{};
	u32 size{};
	u32 cnt{ BIT(0) | BIT(8) };
};

u32 ipc_fifo_recv(NDS *nds, int cpuid);
void ipc_fifo_send(NDS *nds, int cpuid, u32 value);
void ipc_fifo_cnt_write(NDS *nds, int cpuid, u16 value);

} // namespace twice

#endif
