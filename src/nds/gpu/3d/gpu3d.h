#ifndef TWICE_GPU3D_H
#define TWICE_GPU3D_H

#include "common/ringbuf.h"
#include "common/types.h"
#include "nds/gpu/3d/ge.h"
#include "nds/gpu/3d/gpu3d_types.h"
#include "nds/gpu/3d/re.h"

namespace twice {

struct nds_ctx;

struct gpu_3d_engine {
	u32 gxstat{};
	bool halted{};
	bool render_frame{};

	geometry_engine ge;
	rendering_engine re;

	struct fifo_pipe {
		/* TODO: PIPE */
		static constexpr size_t MAX_BUFFER_SIZE = 260;

		struct fifo_entry {
			u8 cmd{};
			u32 param{};
		};

		ringbuf<fifo_entry, 512> buffer;
	} fifo;

	struct gxfifo {
		u8 cmd[4]{};
		int params_left{};
		std::vector<u32> params{};
	} gxfifo;

	vertex_ram vtx_ram[2]{};
	polygon_ram poly_ram[2]{};

	nds_ctx *nds{};
};

void gpu3d_init(nds_ctx *nds);
u8 gpu_3d_read8(gpu_3d_engine *gpu, u16 offset);
u16 gpu_3d_read16(gpu_3d_engine *gpu, u16 offset);
u32 gpu_3d_read32(gpu_3d_engine *gpu, u16 offset);
void gpu_3d_write8(gpu_3d_engine *gpu, u16 offset, u8 value);
void gpu_3d_write16(gpu_3d_engine *gpu, u16 offset, u16 value);
void gpu_3d_write32(gpu_3d_engine *gpu, u16 offset, u32 value);
void gpu3d_on_vblank(gpu_3d_engine *gpu);
void gpu3d_on_scanline_start(nds_ctx *nds);
void gxfifo_check_irq(gpu_3d_engine *gpu);

} // namespace twice

#endif
