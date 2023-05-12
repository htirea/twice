#ifndef TWICE_IPC_H
#define TWICE_IPC_H

#include "common/types.h"

namespace twice {

struct IpcFifo {
	u32 fifo[16]{};
	u32 read{};
	u32 write{};
	u32 size{};
	u32 cnt{ 1 | 1 << 8 };
};

} // namespace twice

#endif
