#include "nds/nds.h"

#include "nds/arm/arm.h"

#include "common/logger.h"
#include "libtwice/exception.h"

namespace twice {

u32
cartridge_make_chip_id(size_t size)
{
	u8 byte0 = 0xC2;
	u8 byte1;
	if (size >> 20 <= 0x80) {
		byte1 = size >> 20;
		if (byte1 != 0) {
			byte1--;
		}
	} else {
		byte1 = 0x100 - (size >> 28);
	}
	u8 byte2 = 0;
	u8 byte3 = 0;

	return (u32)byte3 << 24 | (u32)byte2 << 16 | (u32)byte1 << 8 | byte0;
}

cartridge::cartridge(u8 *data, size_t size)
	: data(data),
	  size(size),
	  read_mask(std::bit_ceil(size) - 1),
	  chip_id(cartridge_make_chip_id(size))
{
}

static void
finish_rom_transfer(nds_ctx *nds)
{
	nds->romctrl &= ~BIT(31);
	nds->romctrl &= ~BIT(23);
	if (nds->auxspicnt & BIT(14)) {
		request_interrupt(nds->cpu[nds->nds_slot_cpu], 19);
	}
}

void
event_advance_rom_transfer(nds_ctx *nds)
{
	auto& cart = nds->cart;
	auto& t = nds->cart.transfer;

	nds->romctrl |= BIT(23);

	switch (t.command[0]) {
	case 0xB7:
	{
		u32 offset = (t.start_addr & ~0xFFF) |
		             ((t.start_addr + t.bytes_read) & 0xFFF);
		if (offset < cart.size) {
			t.bus_data_r = readarr<u32>(cart.data, offset);
		} else {
			t.bus_data_r = -1;
		}
		break;
	}
	case 0xB8:
		t.bus_data_r = cart.chip_id;
		break;
	default:
		t.bus_data_r = -1;
	}

	if (t.transfer_size == 0) {
		finish_rom_transfer(nds);
		return;
	}
}

void
cartridge_start_command(nds_ctx *nds, int cpuid)
{
	auto& cart = nds->cart;
	auto& t = nds->cart.transfer;

	std::memcpy(t.command, nds->cart_command_out, 8);

	u32 bs_bits = nds->romctrl >> 24 & 7;
	if (bs_bits == 0) {
		t.transfer_size = 0;
	} else if (bs_bits < 7) {
		t.transfer_size = 0x100 << bs_bits;
	} else {
		t.transfer_size = 4;
	}

	t.bytes_read = 0;

	switch (t.command[0]) {
	case 0xB7:
		t.start_addr = (u32)t.command[1] << 24 |
		               (u32)t.command[2] << 16 |
		               (u32)t.command[3] << 8 | t.command[4];
		t.start_addr &= cart.read_mask;
		if (t.start_addr < 0x8000) {
			t.start_addr = 0x8000 + (t.start_addr & 0x1FF);
		}
		break;
	case 0xB8:
		break;
	default:
		LOG("unhandled command %016lX\n",
				byteswap64(readarr<u64>(t.command, 0)));
	}

	u32 cycles_per_byte = nds->romctrl & BIT(27) ? 8 : 5;

	schedule_nds_event_after(nds, cpuid,
			event_scheduler::ROM_ADVANCE_TRANSFER,
			cycles_per_byte * 8);

	nds->romctrl |= BIT(31);
	nds->romctrl &= ~BIT(23);
}

u32
read_cart_bus_data(nds_ctx *nds, int cpuid)
{
	if (cpuid != nds->nds_slot_cpu) return 0;

	auto& t = nds->cart.transfer;

	t.bytes_read += 4;
	if (t.bytes_read < t.transfer_size) {
		u32 cycles_per_byte = nds->romctrl & BIT(27) ? 8 : 5;
		schedule_nds_event_after(nds, cpuid,
				event_scheduler::ROM_ADVANCE_TRANSFER,
				cycles_per_byte * 4);
		nds->romctrl &= ~BIT(23);
	} else if (t.bytes_read == t.transfer_size) {
		finish_rom_transfer(nds);
	} else {
		t.bytes_read = t.transfer_size;
	}

	return t.bus_data_r;
}

} // namespace twice
