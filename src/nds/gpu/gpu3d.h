#ifndef TWICE_GPU_3D_H
#define TWICE_GPU_3D_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct gxfifo {
	static constexpr size_t MAX_BUFFER_SIZE = 260;

	struct fifo_entry {
		u8 command{};
		u32 param{};
	};

	std::queue<fifo_entry> buffer;
	u32 params[32]{};
};

struct gpu_3d_engine {
	gpu_3d_engine(nds_ctx *nds);

	u32 gxstat{};
	u16 disp3dcnt{};
	gxfifo fifo;

	struct {
		u32 count{};
		u32 packed{};
		u32 num_params[4]{};
		u32 total_params{};
		std::vector<u32> params;
		u8 commands[4]{};
	} cmd_fifo;

	nds_ctx *nds{};
};

void gxfifo_check_irq(gpu_3d_engine *gpu);

u32 gpu_3d_read32(gpu_3d_engine *gpu, u16 offset);
u16 gpu_3d_read16(gpu_3d_engine *gpu, u16 offset);
u8 gpu_3d_read8(gpu_3d_engine *gpu, u16 offset);
void gpu_3d_write32(gpu_3d_engine *gpu, u16 offset, u32 value);
void gpu_3d_write16(gpu_3d_engine *gpu, u16 offset, u16 value);
void gpu_3d_write8(gpu_3d_engine *gpu, u16 offset, u8 value);

} // namespace twice

#endif
