#ifndef TWICE_GPU3D_H
#define TWICE_GPU3D_H

#include "common/types.h"

#include "nds/gpu/3d/matrix.h"

namespace twice {

struct nds_ctx;

struct gxfifo {
	static constexpr size_t MAX_BUFFER_SIZE = 260;

	struct fifo_entry {
		u8 command{};
		u32 param{};
	};

	std::queue<fifo_entry> buffer;
};

struct rendering_engine {
	struct registers {
		u16 disp3dcnt;
	};

	registers shadow;
	registers r;
};

struct gpu_3d_engine {
	gpu_3d_engine(nds_ctx *nds);

	rendering_engine re;

	u32 gxstat{};
	gxfifo fifo;
	bool halted{};

	struct {
		u32 count{};
		u32 packed{};
		u32 num_params[4]{};
		u32 total_params{};
		std::vector<u32> params;
		u8 commands[4]{};
	} cmd_fifo;

	u32 cmd_params[32]{};

	ge_matrix projection_mtx;
	ge_matrix position_mtx;
	ge_matrix vector_mtx;
	ge_matrix texture_mtx;
	ge_matrix clip_mtx;
	u32 mtx_mode{};
	ge_matrix projection_stack[1]{};
	ge_matrix position_stack[32]{};
	ge_matrix vector_stack[32]{};
	ge_matrix texture_stack[1]{};
	u32 projection_sp{};
	u32 position_sp{};
	u32 texture_sp{};

	nds_ctx *nds{};
};

void gpu3d_on_vblank(gpu_3d_engine *gpu);
void gxfifo_check_irq(gpu_3d_engine *gpu);

u32 gpu_3d_read32(gpu_3d_engine *gpu, u16 offset);
u16 gpu_3d_read16(gpu_3d_engine *gpu, u16 offset);
u8 gpu_3d_read8(gpu_3d_engine *gpu, u16 offset);
void gpu_3d_write32(gpu_3d_engine *gpu, u16 offset, u32 value);
void gpu_3d_write16(gpu_3d_engine *gpu, u16 offset, u16 value);
void gpu_3d_write8(gpu_3d_engine *gpu, u16 offset, u8 value);

void ge_execute_command(gpu_3d_engine *gpu, u8 cmd);

} // namespace twice

#endif
