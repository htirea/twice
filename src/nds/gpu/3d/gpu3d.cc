#include "nds/nds.h"

#include "nds/arm/arm.h"

#include "common/logger.h"

namespace twice {

gpu_3d_engine::gpu_3d_engine(nds_ctx *nds) : nds(nds) {}

static void gxfifo_run_commands(gpu_3d_engine *gpu);
static void execute_swap_buffers(gpu_3d_engine *gpu);

void
gpu3d_on_vblank(gpu_3d_engine *gpu)
{
	if (gpu->halted) {
		execute_swap_buffers(gpu);
		gpu->halted = false;
		gxfifo_run_commands(gpu);
		gpu->render_frame = true;
	}

	if (gpu->re.manual_sort != (bool)(gpu->ge.swap_bits & 1) ||
			gpu->re.r != gpu->re.shadow) {
		gpu->render_frame = true;
	}

	gpu->re.manual_sort = gpu->ge.swap_bits & 1;
	gpu->ge.swap_bits = gpu->ge.swap_bits_s;
	gpu->re.r = gpu->re.shadow;
}

static void
execute_swap_buffers(gpu_3d_engine *gpu)
{
	gpu->ge_buf ^= 1;
	gpu->re_buf ^= 1;

	/* TODO: add delay */
	gpu->vtx_ram[gpu->ge_buf].count = 0;
	gpu->poly_ram[gpu->ge_buf].count = 0;
}

void
gpu3d_on_scanline_start(nds_ctx *nds)
{
	gxfifo_run_commands(&nds->gpu3d);

	if (nds->vcount == 214) {
		if (nds->gpu3d.render_frame) {
			gpu3d_render_frame(&nds->gpu3d);
			nds->gpu3d.render_frame = false;
		}
	}
}

void
gxfifo_check_irq(gpu_3d_engine *gpu)
{
	auto& fifo = gpu->fifo;

	switch (gpu->gxstat >> 30) {
	case 1:
		if (fifo.buffer.size() < 128) {
			request_interrupt(gpu->nds->cpu[0], 21);
		}
		break;
	case 2:
		if (fifo.buffer.size() == 0) {
			request_interrupt(gpu->nds->cpu[0], 21);
		}
		break;
	}
}

static u32 gxstat_read(gpu_3d_engine *gpu);

u8
gpu_3d_read8(gpu_3d_engine *gpu, u16 offset)
{
	switch (offset) {
	case 0x600:
	case 0x601:
	case 0x602:
	case 0x603:
		return gxstat_read(gpu) >> 8 * (offset & 3);
	}

	LOG("3d engine read 8 at offset %03X\n", offset);
	return 0;
}

u16
gpu_3d_read16(gpu_3d_engine *gpu, u16 offset)
{
	switch (offset) {
	case 0x320:
		return 46;
	case 0x600:
	case 0x602:
		return gxstat_read(gpu) >> 8 * (offset & 3);
	case 0x604:
		return gpu->poly_ram[gpu->ge_buf].count;
	case 0x606:
		return gpu->vtx_ram[gpu->ge_buf].count;
	}

	LOG("3d engine read 16 at offset %03X\n", offset);
	return 0;
}

u32
gpu_3d_read32(gpu_3d_engine *gpu, u16 offset)
{
	if (0x640 <= offset && offset < 0x680) {
		u32 idx = offset >> 2 & 0xF;
		return gpu->clip_mtx.v[idx / 4][idx % 4];
	}

	if (0x680 <= offset && offset < 0x6A4) {
		u32 idx = offset >> 2 & 0xF;
		return gpu->vector_mtx.v[idx / 3][idx % 3];
	}

	switch (offset) {
	case 0x600:
		return gxstat_read(gpu);
	case 0x4A4:
		return 0;
	}

	LOG("3d engine read 32 at offset %03X\n", offset);
	return 0;
}

static u32
gxstat_read(gpu_3d_engine *gpu)
{
	u32 fifo_size = gpu->fifo.buffer.size();
	if (fifo_size > 256) {
		fifo_size = 256;
	}

	gpu->gxstat &= ~0x7FF0000;
	gpu->gxstat |= fifo_size << 16;
	gpu->gxstat |= (u32)(fifo_size < 128) << 25;
	gpu->gxstat |= (u32)(fifo_size == 0) << 26;

	return gpu->gxstat;
}

static void gxstat_write_byte_3(gpu_3d_engine *gpu, u8 value);
static void gxstat_write(gpu_3d_engine *gpu, u32 value);

void
gpu_3d_write8(gpu_3d_engine *gpu, u16 offset, u8 value)
{
	switch (offset) {
	case 0x603:
		gxstat_write_byte_3(gpu, value);
		return;
	}

	LOG("3d engine write 8 at offset %03X %02X\n", offset, value);
}

void
gpu_3d_write16(gpu_3d_engine *gpu, u16 offset, u16 value)
{
	if (0x330 <= offset && offset < 0x340) {
		writearr<u16>(gpu->re.shadow.edge_color, offset & 0xF, value);
		return;
	}

	if (0x380 <= offset && offset < 0x3C0) {
		writearr<u16>(gpu->re.shadow.toon_table, offset & 0x3F, value);
		return;
	}

	switch (offset) {
	case 0x340:
		gpu->re.shadow.alpha_test_ref = value & 0x1F;
		return;
	case 0x354:
		gpu->re.shadow.clear_depth = value & 0x7FFF;
		return;
	case 0x356:
		gpu->re.shadow.clrimage_offset = value;
		return;
	case 0x35C:
		gpu->re.shadow.fog_offset = value & 0x7FFF;
		return;
	case 0x610:
		gpu->ge.disp_1dot_depth = (s32)(value & 0x7FFF) << 9;
		return;
	}

	LOG("3d engine write 16 at offset %03X, value %04X\n", offset, value);
}

static void gxfifo_write(gpu_3d_engine *gpu, u32 value);
static bool valid_command(u8 command);
static void gxfifo_push(
		gpu_3d_engine *gpu, u8 command, u32 param, bool run_commands);

void
gpu_3d_write32(gpu_3d_engine *gpu, u16 offset, u32 value)
{
	if (0x330 <= offset && offset < 0x340) {
		writearr<u32>(gpu->re.shadow.edge_color, offset & 0xF, value);
		return;
	}

	if (0x360 <= offset && offset < 0x380) {
		writearr<u32>(gpu->re.shadow.fog_table, offset & 0x1F, value);
		return;
	}

	if (0x380 <= offset && offset < 0x3C0) {
		writearr<u32>(gpu->re.shadow.toon_table, offset & 0x3F, value);
		return;
	}

	if (0x400 <= offset && offset < 0x440) {
		gxfifo_write(gpu, value);
		return;
	}

	u8 command = offset >> 2 & 0xFF;
	if (valid_command(command)) {
		gxfifo_push(gpu, command, value, true);
		return;
	}

	switch (offset) {
	case 0x350:
		gpu->re.shadow.clear_color = value & 0x3F1FFFFF;
		return;
	case 0x358:
		gpu->re.shadow.fog_color = value & 0x1F7FFF;
		return;
	case 0x35C:
		gpu->re.shadow.fog_offset = value & 0x7FFF;
		return;
	case 0x600:
		gxstat_write(gpu, value);
		return;
	}

	LOG("3d engine write 32 at offset %03X, value %08X\n", offset, value);
}

static void
gxstat_write_byte_3(gpu_3d_engine *gpu, u8 value)
{
	gpu->gxstat = (gpu->gxstat & ~0xC0000000) |
	              ((u32)value << 24 & 0xC0000000);
	gxfifo_check_irq(gpu);
}

static void
gxstat_write(gpu_3d_engine *gpu, u32 value)
{
	gpu->gxstat = (gpu->gxstat & ~0xC0000000) | (value & 0xC0000000);
	if (value & BIT(15)) {
		gpu->gxstat &= ~BIT(15);
		gpu->gxstat &= ~BIT(13);
		gpu->projection_sp = 0;
		gpu->texture_sp = 0;
	}

	gxfifo_check_irq(gpu);
}

static void unpack_and_run_commands(gpu_3d_engine *gpu);
static u32 get_num_params(u8 cmd);

static void
gxfifo_write(gpu_3d_engine *gpu, u32 value)
{
	auto& cfifo = gpu->cmd_fifo;

	if (cfifo.count == 0) {
		cfifo.packed = value;
		cfifo.total_params = 0;
		for (int i = 0; i < 4; i++) {
			u8 command = cfifo.packed >> 8 * i;
			if (!valid_command(command)) {
				LOG("invalid command %02X\n", command);
				cfifo.num_params[i] = 0;
				cfifo.commands[i] = 0;
			} else {
				cfifo.commands[i] = command;
				cfifo.num_params[i] = get_num_params(command);
			}
		}
		cfifo.total_params = cfifo.num_params[0] +
		                     cfifo.num_params[1] +
		                     cfifo.num_params[2] + cfifo.num_params[3];
		cfifo.params.clear();
		cfifo.count++;

		if (cfifo.total_params == 0) {
			unpack_and_run_commands(gpu);
		}
	} else if (cfifo.count < cfifo.total_params) {
		cfifo.params.push_back(value);
		cfifo.count++;
	} else if (cfifo.count == cfifo.total_params) {
		cfifo.params.push_back(value);
		unpack_and_run_commands(gpu);
	} else {
		throw twice_error("bad cmd gxfifo state");
	}
}

static void
unpack_and_run_commands(gpu_3d_engine *gpu)
{
	auto& cfifo = gpu->cmd_fifo;
	cfifo.count = 0;

	u32 param_idx = 0;
	for (int i = 0; i < 4; i++) {
		u8 command = cfifo.commands[i];
		if (!valid_command(command)) {
			throw twice_error("invalid command");
		}
		u32 num_params = cfifo.num_params[i];
		if (num_params == 0) {
			gxfifo_push(gpu, command, 0, false);
		} else {
			for (; num_params--; param_idx++) {
				gxfifo_push(gpu, command,
						cfifo.params[param_idx],
						false);
			}
		}
	}
	gxfifo_run_commands(gpu);
}

static u32
get_num_params(u8 cmd)
{
	switch (cmd) {
	case 0x0:
		return 0;
	case 0x11:
	case 0x15:
	case 0x41:
		return 0;
	case 0x16:
	case 0x18:
		return 16;
	case 0x17:
	case 0x19:
		return 12;
	case 0x1A:
		return 9;
	case 0x1B:
	case 0x1C:
	case 0x70:
		return 3;
	case 0x23:
	case 0x71:
		return 2;
	case 0x34:
		return 32;
	default:
		return 1;
	}
}

static bool
valid_command(u8 command)
{
	return (command == 0) ||                       //
	       (0x10 <= command && command <= 0x1C) || //
	       (0x20 <= command && command <= 0x2B) || //
	       (0x30 <= command && command <= 0x34) || //
	       (0x40 <= command && command <= 0x41) || //
	       (command == 0x50) ||                    //
	       (command == 0x60) ||                    //
	       (0x70 <= command && command <= 0x72);
}

static void
gxfifo_push(gpu_3d_engine *gpu, u8 command, u32 param, bool run_commands)
{
	auto& fifo = gpu->fifo;

	if (fifo.buffer.size() >= gpu_3d_engine::gxfifo::MAX_BUFFER_SIZE) {
		halt_cpu(gpu->nds->cpu[0], arm_cpu::CPU_GXFIFO_HALT);
	}

	fifo.buffer.push({ command, param });
	if (run_commands) {
		gxfifo_run_commands(gpu);
	}
}

static void
gxfifo_run_commands(gpu_3d_engine *gpu)
{
	if (gpu->halted) {
		return;
	}

	auto& fifo = gpu->fifo;

	while (!fifo.buffer.empty()) {
		auto entry = fifo.buffer.front();
		u32 num_params = get_num_params(entry.command);
		if (num_params == 0) {
			fifo.buffer.pop();
			ge_execute_command(gpu, entry.command);
		} else if (fifo.buffer.size() >= num_params) {
			for (u32 i = 0; i < num_params; i++) {
				gpu->cmd_params[i] = fifo.buffer.pop().param;
			}
			ge_execute_command(gpu, entry.command);
		} else {
			break;
		}
	}

	gxfifo_check_irq(gpu);

	if (fifo.buffer.size() < 128) {
		start_gxfifo_dmas(gpu->nds);
	}

	if (fifo.buffer.size() < gpu_3d_engine::gxfifo::MAX_BUFFER_SIZE) {
		unhalt_cpu(gpu->nds->cpu[0], arm_cpu::CPU_GXFIFO_HALT);
	}
}

} // namespace twice
