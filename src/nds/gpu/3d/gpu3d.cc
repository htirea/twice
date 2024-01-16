#include "nds/gpu/3d/gpu3d.h"

#include "nds/arm/arm.h"
#include "nds/nds.h"

namespace twice {

static const int ge_cmd_num_params[0x100] = {
	0, -1, -1, -1, -1, -1, -1, -1,  // 0x00
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x08
	1, 0, 1, 1, 1, 0, 16, 12,       // 0x10
	16, 12, 9, 3, 3, -1, -1, -1,    // 0x18
	1, 1, 1, 2, 1, 1, 1, 1,         // 0x20
	1, 1, 1, 1, -1, -1, -1, -1,     // 0x28
	1, 1, 1, 1, 32, -1, -1, -1,     // 0x30
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x38
	1, 0, -1, -1, -1, -1, -1, -1,   // 0x40
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x48
	1, -1, -1, -1, -1, -1, -1, -1,  // 0x50
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x58
	1, -1, -1, -1, -1, -1, -1, -1,  // 0x60
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x68
	3, 2, 1, -1, -1, -1, -1, -1,    // 0x70
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x78
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x80
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x88
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x90
	-1, -1, -1, -1, -1, -1, -1, -1, // 0x98
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xA0
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xA8
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xB0
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xB8
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xC0
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xC8
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xD0
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xD8
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xE0
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xE8
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xF0
	-1, -1, -1, -1, -1, -1, -1, -1, // 0xF8
};

static u32 gxstat_read(gpu_3d_engine *gpu);
static void gxstat_write(gpu_3d_engine *gpu, u32 value);
static void gxstat_write_byte_3(gpu_3d_engine *gpu, u8 value);
static void gxfifo_write(gpu_3d_engine *gpu, u32 value);
static void unpack_gxfifo_to_fifo(gpu_3d_engine *gpu);
static void fifo_pipe_push(gpu_3d_engine *gpu, u8 cmd, u32 param);
static void fifo_pipe_process_commands(gpu_3d_engine *gpu);
static void execute_swap_buffers(gpu_3d_engine *gpu);
static bool valid_ge_command(u8 command);

void
gpu3d_init(nds_ctx *nds)
{
	gpu_3d_engine *gpu = &nds->gpu3d;
	gpu->nds = nds;

	gpu->ge.gpu = gpu;
	gpu->ge.vtx_ram = &gpu->vtx_ram[0];
	gpu->ge.poly_ram = &gpu->poly_ram[0];

	gpu->re.gpu = gpu;
	gpu->re.vtx_ram = &gpu->vtx_ram[1];
	gpu->re.poly_ram = &gpu->poly_ram[1];
}

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
		return gpu->ge.poly_ram->count;
	case 0x606:
		return gpu->ge.vtx_ram->count;
	}

	LOG("3d engine read 16 at offset %03X\n", offset);
	return 0;
}

u32
gpu_3d_read32(gpu_3d_engine *gpu, u16 offset)
{
	if (0x640 <= offset && offset < 0x680) {
		u32 idx = offset >> 2 & 0xF;
		return gpu->ge.clip_mtx[idx];
	}

	if (0x680 <= offset && offset < 0x6A4) {
		u32 idx = offset >> 2 & 0xF;
		return gpu->ge.vector_mtx[((idx / 3) << 2) + idx % 3];
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
		gpu->re.r_s._edge_color[offset >> 1 & 7] = value;
		return;
	}

	if (0x380 <= offset && offset < 0x3C0) {
		gpu->re.r_s._toon_table[offset >> 1 & 0x1F] = value;
		return;
	}

	switch (offset) {
	case 0x340:
		gpu->re.r_s.alpha_test_ref = value & 0x1F;
		return;
	case 0x354:
		gpu->re.r_s.clear_depth = value & 0x7FFF;
		return;
	case 0x356:
		gpu->re.r_s.clrimage_offset = value;
		return;
	case 0x35C:
		gpu->re.r_s.fog_offset = value & 0x7FFF;
		return;
	case 0x610:
		/* NO SEXT */
		gpu->ge.disp_1dot_depth = (s32)(value & 0x7FFF) << 9;
		return;
	}

	LOG("3d engine write 16 at offset %03X, value %04X\n", offset, value);
}

void
gpu_3d_write32(gpu_3d_engine *gpu, u16 offset, u32 value)
{
	if (0x330 <= offset && offset < 0x340) {
		u32 idx = offset >> 1 & 7;
		gpu->re.r_s._edge_color[idx] = value;
		gpu->re.r_s._edge_color[idx + 1] = value >> 16;
		return;
	}

	if (0x360 <= offset && offset < 0x380) {
		writearr<u32>(gpu->re.r_s.fog_table.data(), offset & 0x1F,
				value);
		return;
	}

	if (0x380 <= offset && offset < 0x3C0) {
		u32 idx = offset >> 1 & 0x1F;
		gpu->re.r_s._toon_table[idx] = value;
		gpu->re.r_s._toon_table[idx + 1] = value >> 16;
		return;
	}

	if (0x400 <= offset && offset < 0x440) {
		gxfifo_write(gpu, value);
		return;
	}

	if (0x440 <= offset && offset < 0x600) {
		u8 command = offset >> 2 & 0xFF;
		if (valid_ge_command(command)) {
			fifo_pipe_push(gpu, command, value);
			fifo_pipe_process_commands(gpu);
			return;
		}
	}

	switch (offset) {
	case 0x350:
		gpu->re.r_s.clear_color = value & 0x3F1FFFFF;
		return;
	case 0x358:
		gpu->re.r_s.fog_color = value & 0x1F7FFF;
		return;
	case 0x35C:
		gpu->re.r_s.fog_offset = value & 0x7FFF;
		return;
	case 0x600:
		gxstat_write(gpu, value);
		return;
	}

	LOG("3d engine write 32 at offset %03X, value %08X\n", offset, value);
}

void
gpu3d_on_vblank(gpu_3d_engine *gpu)
{
	if (gpu->halted) {
		execute_swap_buffers(gpu);
		gpu->render_frame = true;
	}

	if (gpu->re.manual_sort != (bool)(gpu->ge.swap_bits & 1) ||
			gpu->re.r != gpu->re.r_s ||
			gpu->nds->vram.texture_changed ||
			gpu->nds->vram.texture_palette_changed) {
		gpu->render_frame = true;
	}

	gpu->re.manual_sort = gpu->ge.swap_bits & 1;
	gpu->re.r = gpu->re.r_s;
	fifo_pipe_process_commands(gpu);
}

void
gpu3d_on_scanline_start(nds_ctx *nds)
{
	fifo_pipe_process_commands(&nds->gpu3d);

	if (nds->vcount == 214) {
		if (nds->gpu3d.render_frame) {
			re_render_frame(&nds->gpu3d.re);
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

static u32
gxstat_read(gpu_3d_engine *gpu)
{
	/* TODO: unhandled bits when gpu timings added */
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

static void
gxstat_write(gpu_3d_engine *gpu, u32 value)
{
	gpu->gxstat = (gpu->gxstat & ~0xC0000000) | (value & 0xC0000000);

	if (value & BIT(15)) {
		gpu->gxstat &= ~BIT(15);
		gpu->gxstat &= ~BIT(13);
		gpu->ge.proj_sp = 0;
		gpu->ge.texture_sp = 0;
	}

	gxfifo_check_irq(gpu);
}

static void
gxstat_write_byte_3(gpu_3d_engine *gpu, u8 value)
{
	gpu->gxstat = (gpu->gxstat & ~0xC0000000) |
	              ((u32)(value << 24) & 0xC0000000);
	gxfifo_check_irq(gpu);
}

static void
gxfifo_write(gpu_3d_engine *gpu, u32 value)
{
	auto& gxfifo = gpu->gxfifo;

	if (gxfifo.params_left == 0) {
		for (u32 i = 0; i < 4; i++) {
			u8 cmd = value >> 8 * i;
			if (!valid_ge_command(cmd)) {
				LOG("invalid packed command %02X\n", cmd);
				cmd = 0;
			}

			gxfifo.cmd[i] = cmd;
			gxfifo.params_left += ge_cmd_num_params[cmd];
		}

		if (gxfifo.params_left == 0) {
			unpack_gxfifo_to_fifo(gpu);
		}

		gxfifo.params.clear();
	} else {
		gxfifo.params_left--;
		gxfifo.params.push_back(value);

		if (gxfifo.params_left == 0) {
			unpack_gxfifo_to_fifo(gpu);
		}
	}
}

static void
unpack_gxfifo_to_fifo(gpu_3d_engine *gpu)
{
	auto& gxfifo = gpu->gxfifo;
	u32 param_idx = 0;

	for (u32 i = 0; i < 4; i++) {
		u8 cmd = gxfifo.cmd[i];
		u32 num_params = ge_cmd_num_params[cmd];

		if (num_params == 0) {
			fifo_pipe_push(gpu, cmd, 0);
		} else {
			for (; num_params--; param_idx++) {
				u32 param = gxfifo.params[param_idx];
				fifo_pipe_push(gpu, cmd,
						gxfifo.params[param_idx]);
			}
		}
	}

	fifo_pipe_process_commands(gpu);
}

static void
fifo_pipe_push(gpu_3d_engine *gpu, u8 cmd, u32 param)
{
	/* TODO: it should stop the cpu, not halt it
	 * note: need to fix the current stop implementation
	 */

	auto& fifo = gpu->fifo;

	if (fifo.buffer.size() >= gpu_3d_engine::fifo_pipe::MAX_BUFFER_SIZE) {
		halt_cpu(gpu->nds->cpu[0], arm_cpu::CPU_GXFIFO_HALT);
	}

	fifo.buffer.push({ cmd, param });
}

static void
fifo_pipe_process_commands(gpu_3d_engine *gpu)
{
	/* TODO: gpu timings */

	if (gpu->halted)
		return;

	auto& fifo = gpu->fifo;

	while (!fifo.buffer.empty()) {
		auto entry = fifo.buffer.front();
		u32 num_params = ge_cmd_num_params[entry.cmd];

		if (num_params == 0) {
			fifo.buffer.pop();
			ge_execute_command(&gpu->ge, entry.cmd);
		} else if (fifo.buffer.size() >= num_params) {
			for (u32 i = 0; i < num_params; i++) {
				gpu->ge.cmd_params[i] =
						fifo.buffer.pop().param;
			}
			ge_execute_command(&gpu->ge, entry.cmd);
		} else {
			break;
		}
	}

	gxfifo_check_irq(gpu);

	if (fifo.buffer.size() < 128) {
		start_gxfifo_dmas(gpu->nds);
	}

	if (fifo.buffer.size() < gpu_3d_engine::fifo_pipe::MAX_BUFFER_SIZE) {
		unhalt_cpu(gpu->nds->cpu[0], arm_cpu::CPU_GXFIFO_HALT);
	}
}

static void
execute_swap_buffers(gpu_3d_engine *gpu)
{
	std::swap(gpu->ge.vtx_ram, gpu->re.vtx_ram);
	std::swap(gpu->ge.poly_ram, gpu->re.poly_ram);
	gpu->ge.vtx_ram->count = 0;
	gpu->ge.poly_ram->count = 0;
	gpu->halted = false;
}

static bool
valid_ge_command(u8 command)
{
	return ge_cmd_num_params[command] != -1;
}

} // namespace twice
