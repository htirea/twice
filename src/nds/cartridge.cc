#include "nds/nds.h"

#include "nds/arm/arm.h"

#include "common/logger.h"

namespace twice {

cartridge::cartridge(u8 *data, size_t size)
	: data(data),
	  size(size)
{
}

void
cartridge_start_command(nds_ctx *nds, int cpuid)
{
	auto& cart = nds->cart;

	u64 command = readarr<u64>(nds->cart_command_out, 0);
	command = byteswap64(command);
	u32 bs_bits = nds->romctrl >> 24 & 7;
	LOG("cart command %016lX, bits: %u\n", command, bs_bits);
	if (bs_bits == 0) {
		cart.bus_transfer_bytes_left = 0;
		nds->romctrl &= ~BIT(31);
		if (nds->auxspicnt & BIT(14)) {
			request_interrupt(nds->cpu[cpuid], 19);
		}
	} else if (bs_bits < 7) {
		cart.bus_transfer_bytes_left = 0x100 << bs_bits;
	} else {
		cart.bus_transfer_bytes_left = 4;
	}
}

u32
read_cart_bus_data(nds_ctx *nds, int cpuid)
{
	if (cpuid != nds->nds_slot_cpu) return 0;

	auto& cart = nds->cart;

	cart.bus_transfer_bytes_left -= 4;
	if (cart.bus_transfer_bytes_left == 0) {
		nds->romctrl &= ~BIT(31);
		if (nds->auxspicnt & BIT(14)) {
			request_interrupt(nds->cpu[cpuid], 19);
		}
	} else if (cart.bus_transfer_bytes_left < 0) {
		cart.bus_transfer_bytes_left = 0;
	}

	return -1;
}

u32
cartridge_make_chip_id(nds_ctx *nds)
{
	auto& cart = nds->cart;

	u8 byte0 = 0xC2;
	u8 byte1;
	if (cart.size >> 20 <= 0x80) {
		byte1 = cart.size >> 20;
		if (byte1 != 0) {
			byte1--;
		}
	} else {
		byte1 = 0x100 - (cart.size >> 28);
	}
	u8 byte2 = 0;
	u8 byte3 = 0;

	return (u32)byte3 << 24 | (u32)byte2 << 16 | (u32)byte1 << 8 | byte0;
}

} // namespace twice
