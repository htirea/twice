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
	  size(size)
{
	chip_id = cartridge_make_chip_id(size);
	if (!std::has_single_bit(size) && size != 0) {
		throw twice_error("cartridge size not a power of 2");
	}
}

static void
do_cart_command(nds_ctx *nds)
{
	auto& cart = nds->cart;

	switch (cart.command >> 56) {
	case 0xB7:
	{
		cart.start_addr = cart.command >> 24 & 0xFFFFFFFF;
		cart.addr_offset = 0;
		break;
	}
	case 0xB8:
		break;
	default:
		LOG("unhandled command %016lX\n", cart.command);
	}
}

void
cartridge_start_command(nds_ctx *nds, int cpuid)
{
	auto& cart = nds->cart;

	nds->romctrl |= BIT(23);

	cart.command = byteswap64(readarr<u64>(nds->cart_command_out, 0));
	u32 bs_bits = nds->romctrl >> 24 & 7;
	if (bs_bits == 0) {
		cart.bus_transfer_bytes_left = 0;
		nds->romctrl &= ~BIT(31);
		nds->romctrl &= ~BIT(23);
		if (nds->auxspicnt & BIT(14)) {
			request_interrupt(nds->cpu[cpuid], 19);
		}
	} else if (bs_bits < 7) {
		cart.bus_transfer_bytes_left = 0x100 << bs_bits;
	} else {
		cart.bus_transfer_bytes_left = 4;
	}

	do_cart_command(nds);
}

u32
read_cart_bus_data(nds_ctx *nds, int cpuid)
{
	if (cpuid != nds->nds_slot_cpu) return 0;

	auto& cart = nds->cart;

	cart.bus_transfer_bytes_left -= 4;
	if (cart.bus_transfer_bytes_left == 0) {
		nds->romctrl &= ~BIT(31);
		nds->romctrl &= ~BIT(23);
		if (nds->auxspicnt & BIT(14)) {
			request_interrupt(nds->cpu[cpuid], 19);
		}
	} else if (cart.bus_transfer_bytes_left < 0) {
		cart.bus_transfer_bytes_left = 0;
	}

	switch (cart.command >> 56) {
	case 0xB7:
	{
		u32 offset = (cart.start_addr + cart.addr_offset) &
		             (cart.size - 1);
		if (offset < 0x8000) {
			offset = 0x8000 + (offset & 0x1FF);
		}
		cart.addr_offset = cart.addr_offset + 4 & 0x1FF;
		return readarr<u32>(cart.data, offset);
	}
	case 0xB8:
		return cart.chip_id;
	}

	return -1;
}

} // namespace twice
