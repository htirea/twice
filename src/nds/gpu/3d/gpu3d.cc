#include "nds/nds.h"

#include "nds/arm/arm.h"

#include "common/logger.h"

namespace twice {

gpu_3d_engine::gpu_3d_engine(nds_ctx *nds)
	: nds(nds)
{
}

static void
update_gxstat_fifo_bits(gpu_3d_engine *gpu)
{
	u32 fifo_size = gpu->fifo.buffer.size();
	if (fifo_size > 256) {
		fifo_size = 256;
	}

	gpu->gxstat &= ~0x7FF0000;
	gpu->gxstat |= fifo_size << 16;
	gpu->gxstat |= (u32)(fifo_size < 128) << 25;
	gpu->gxstat |= (u32)(fifo_size == 0) << 26;
}

static u32
gxstat_read(gpu_3d_engine *gpu)
{
	update_gxstat_fifo_bits(gpu);
	return gpu->gxstat;
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

	gxfifo_check_irq(gpu);
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

static void
gxfifo_run_commands(gpu_3d_engine *gpu)
{
	auto& fifo = gpu->fifo;

	while (!fifo.buffer.empty()) {
		gxfifo::fifo_entry entry = fifo.buffer.front();
		u32 num_params = get_num_params(entry.command);
		if (num_params == 0) {
			fifo.buffer.pop();
			ge_execute_command(gpu, entry.command);
		} else if (fifo.buffer.size() >= num_params) {
			for (u32 i = 0; i < num_params; i++) {
				gpu->cmd_params[i] = fifo.buffer.front().param;
				fifo.buffer.pop();
			}
			ge_execute_command(gpu, entry.command);
		} else {
			break;
		}
	}

	gxfifo_check_irq(gpu);
}

static void
gxfifo_push(gpu_3d_engine *gpu, u8 command, u32 param, bool run_commands)
{
	auto& fifo = gpu->fifo;

	if (fifo.buffer.size() >= gxfifo::MAX_BUFFER_SIZE) {
		LOG("gxfifo buffer full\n");
	}

	fifo.buffer.push({ command, param });
	if (run_commands) {
		gxfifo_run_commands(gpu);
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
		for (; num_params--; param_idx++) {
			gxfifo_push(gpu, command, cfifo.params[param_idx],
					false);
		}
	}
	gxfifo_run_commands(gpu);
}

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

u32
gpu_3d_read32(gpu_3d_engine *gpu, u16 offset)
{
	switch (offset) {
	case 0x600:
		return gxstat_read(gpu);
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
	u8 command = offset >> 2 & 0xFF;
	if (valid_command(command)) {
		gxfifo_push(gpu, command, value, true);
		return;
	}

	switch (offset) {
	case 0x400:
		gxfifo_write(gpu, value);
		break;
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
