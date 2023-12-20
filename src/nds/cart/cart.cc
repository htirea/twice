#include "nds/nds.h"

#include "nds/arm/arm.h"
#include "nds/cart/key.h"

#include "common/logger.h"
#include "libtwice/exception.h"
#include "libtwice/nds/game_db.h"

namespace twice {

static void start_cartridge_command(nds_ctx *, int cpuid);
static void finish_rom_transfer(nds_ctx *);
static u32 cartridge_make_chip_id(size_t size);

void
cartridge_init(nds_ctx *nds, u8 *data, size_t size, u8 *save_data,
		size_t save_size, int savetype, u8 *arm7_bios)
{
	auto& cart = nds->cart;
	cart.data = data;
	cart.size = size;
	cart.read_mask = std::bit_ceil(size) - 1;
	cart.chip_id = cartridge_make_chip_id(size);
	cart.gamecode = readarr_checked<u32>(data, 0xC, size, 0);
	cart.infrared = (cart.gamecode & 0xFF) == 'I';

	cartridge_backup_init(nds, save_data, save_size, savetype);

	for (u32 i = 0; i < 0x412; i++) {
		cart.keybuf_s[i] = readarr<u32>(arm7_bios, 0x30 + i * 4);
	}
}

void
romctrl_write(nds_ctx *nds, int cpuid, u32 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	bool slot_enabled = nds->auxspicnt & BIT(15);
	bool serial_mode = nds->auxspicnt & BIT(13);
	bool start_command = ~nds->romctrl & value & BIT(31);

	nds->romctrl = (nds->romctrl & 0xA0800000) | (value & ~0x80800000);
	if (start_command && slot_enabled && !serial_mode) {
		start_cartridge_command(nds, cpuid);
	}
}

u32
read_cart_bus_data(nds_ctx *nds, int cpuid)
{
	if (cpuid != nds->nds_slot_cpu)
		return 0;

	auto& t = nds->cart.transfer;
	t.count += 4;

	if (t.count < t.length) {
		u32 cycles_per_byte = (nds->romctrl & BIT(27) ? 8 : 5) << 1;
		schedule_event_after(nds, cpuid, scheduler::CART_TRANSFER,
				cycles_per_byte * 4);
		nds->romctrl &= ~BIT(23);
	} else if (t.count == t.length) {
		finish_rom_transfer(nds);
	} else {
		t.count = t.length;
	}

	return t.bus_data_r;
}

void
event_advance_rom_transfer(nds_ctx *nds, intptr_t, timestamp late)
{
	auto& cart = nds->cart;
	auto& t = nds->cart.transfer;

	if (t.length == 0) {
		finish_rom_transfer(nds);
		return;
	}

	nds->romctrl |= BIT(23);

	if (t.key1) {
		switch (t.command[7] >> 4) {
		case 0x1:
			t.bus_data_r = cart.chip_id;
			break;
		case 0x2:
		{
			u32 offset = (t.addr & ~0xFFF) |
			             ((t.addr + t.count) & 0xFFF);
			t.bus_data_r = readarr_checked<u32>(
					cart.data, offset, cart.size, -1);
			break;
		}
		case 0xA:
			t.bus_data_r = 0;
			break;
		default:
			t.bus_data_r = -1;
		}
	} else {
		switch (t.command[7]) {
		case 0x00:
			t.bus_data_r = readarr_checked<u32>(cart.data,
					t.count & 0xFFF, cart.size, -1);
			break;
		case 0xB7:
		{
			u32 offset = (t.addr & ~0xFFF) |
			             ((t.addr + t.count) & 0xFFF);
			t.bus_data_r = readarr_checked<u32>(
					cart.data, offset, cart.size, -1);
			break;
		}
		case 0x90:
		case 0xB8:
			t.bus_data_r = cart.chip_id;
			break;
		case 0x9F:
		case 0x3C:
		default:
			t.bus_data_r = -1;
		}
	}

	start_cartridge_dmas(nds, nds->nds_slot_cpu);
}

static void
start_cartridge_command(nds_ctx *nds, int cpuid)
{
	auto& cart = nds->cart;
	auto& t = nds->cart.transfer;

	for (u32 i = 0; i < 8; i++) {
		t.command[i] = nds->cart_command_out[7 - i];
	}
	if (t.key1) {
		cart_decrypt64(t.command, cart.keybuf);
	}

	u32 bs_bits = nds->romctrl >> 24 & 7;
	if (bs_bits == 0) {
		t.length = 0;
	} else if (bs_bits < 7) {
		t.length = 0x100 << bs_bits;
	} else {
		t.length = 4;
	}

	t.count = 0;
	t.addr = 0;

	if (t.key1) {
		switch (t.command[7] >> 4) {
		case 0x4:
		case 0x1:
			break;
		case 0xA:
			t.key1 = false;
			break;
		case 0x2:
			t.addr = t.command[5] >> 4 & 7;
			t.addr <<= 12;
			break;
		default:
			LOG("unhandled command %016lX\n",
					readarr<u64>(t.command, 0));
		}
	} else {
		switch (t.command[7]) {
		case 0x9F:
		case 0x00:
		case 0x90:
			break;
		case 0x3C:
			t.key1 = true;
			cart_init_keycode(&cart, cart.gamecode, 2, 8);
			break;
		case 0xB7:
			t.addr = (u32)t.command[6] << 24 |
			         (u32)t.command[5] << 16 |
			         (u32)t.command[4] << 8 | t.command[3];
			t.addr &= cart.read_mask;
			if (t.addr < 0x8000) {
				t.addr = 0x8000 + (t.addr & 0x1FF);
			}
			break;
		case 0xB8:
			break;
		default:
			LOG("unhandled command %016lX\n",
					readarr<u64>(t.command, 0));
		}
	}

	u32 cycles_per_byte = (nds->romctrl & BIT(27) ? 8 : 5) << 1;
	schedule_event_after(nds, cpuid, scheduler::CART_TRANSFER,
			cycles_per_byte * 8);
	nds->romctrl |= BIT(31);
	nds->romctrl &= ~BIT(23);
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

static u32
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

} // namespace twice
