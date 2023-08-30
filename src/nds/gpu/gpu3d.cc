#include "nds/nds.h"

#include "common/logger.h"

namespace twice {

gpu_3d_engine::gpu_3d_engine(nds_ctx *nds)
	: nds(nds)
{
}

static void
update_gxstat_fifo_bits(gpu_3d_engine *gpu)
{
	u32 fifo_size = std::min((u32)256, gpu->fifo.size);
	gpu->gxstat &= ~0x7FF0000;
	gpu->gxstat |= fifo_size << 16;
	gpu->gxstat |= (u32)(fifo_size < 128) << 25;
	gpu->gxstat |= (u32)(fifo_size == 0) << 26;
}

static void
gxstat_write(gpu_3d_engine *gpu, u32 value)
{
	gpu->gxstat = (gpu->gxstat & ~0xC0000000) | (value & 0xC0000000);
	if (value & BIT(15)) {
		gpu->gxstat &= ~BIT(15);
		gpu->gxstat &= ~BIT(13);
		/* TODO: reset texture stack pointer */
	}

	update_gxstat_fifo_bits(gpu);

	/* TODO: FIFO IRQ */
}

u32
gpu_3d_read32(gpu_3d_engine *gpu, u16 offset)
{
	switch (offset) {
	case 0x600:
		return gpu->gxstat;
	}

	LOG("3d engine read 32 at offset %03X\n", offset);
	return 0;
}

u16
gpu_3d_read16(gpu_3d_engine *, u16 offset)
{
	LOG("3d engine read 16 at offset %03X\n", offset);
	return 0;
}

u8
gpu_3d_read8(gpu_3d_engine *, u16 offset)
{
	LOG("3d engine read 8 at offset %03X\n", offset);
	return 0;
}

void
gpu_3d_write32(gpu_3d_engine *gpu, u16 offset, u32 value)
{
	switch (offset) {
	case 0x600:
		gxstat_write(gpu, value);
		return;
	}
	LOG("3d engine write 32 at offset %03X, value %08X\n", offset, value);
}

void
gpu_3d_write16(gpu_3d_engine *, u16 offset, u16 value)
{
	LOG("3d engine write 16 at offset %03X, value %04X\n", offset, value);
}

void
gpu_3d_write8(gpu_3d_engine *, u16 offset, u8 value)
{
	LOG("3d engine write 8 at offset %03X %02X\n", offset, value);
}

} // namespace twice
