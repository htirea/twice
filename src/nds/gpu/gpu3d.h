#ifndef TWICE_GPU_3D_H
#define TWICE_GPU_3D_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct gxfifo {
	struct fifo_entry {
		u8 command{};
		u32 param{};
	};

	fifo_entry buffer[260];
	u32 read{};
	u32 write{};
	u32 size{};
};

struct gpu_3d_engine {
	gpu_3d_engine(nds_ctx *nds);

	u32 gxstat{};
	gxfifo fifo;

	nds_ctx *nds{};
};

u32 gpu_3d_read32(gpu_3d_engine *gpu, u16 offset);
u16 gpu_3d_read16(gpu_3d_engine *gpu, u16 offset);
u8 gpu_3d_read8(gpu_3d_engine *gpu, u16 offset);
void gpu_3d_write32(gpu_3d_engine *gpu, u16 offset, u32 value);
void gpu_3d_write16(gpu_3d_engine *gpu, u16 offset, u16 value);
void gpu_3d_write8(gpu_3d_engine *gpu, u16 offset, u8 value);

} // namespace twice

#endif
