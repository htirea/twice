#include "nds/nds.h"

#include "nds/arm/arm.h"

#include "common/logger.h"
#include "libtwice/exception.h"
#include "libtwice/nds/game_db.h"

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

static void
encrypt64(u8 *p, u32 *keybuf)
{
	u32 y = readarr<u32>(p, 0);
	u32 x = readarr<u32>(p, 4);

	for (u32 i = 0; i <= 0xF; i++) {
		u32 z = keybuf[i] ^ x;
		x = keybuf[0x012 + (z >> 24 & 0xFF)];
		x += keybuf[0x112 + (z >> 16 & 0xFF)];
		x ^= keybuf[0x212 + (z >> 8 & 0xFF)];
		x += keybuf[0x312 + (z & 0xFF)];
		x ^= y;
		y = z;
	}

	writearr<u32>(p, 0, x ^ keybuf[0x10]);
	writearr<u32>(p, 4, y ^ keybuf[0x11]);
}

static void
decrypt64(u8 *p, u32 *keybuf)
{
	u32 y = readarr<u32>(p, 0);
	u32 x = readarr<u32>(p, 4);

	for (u32 i = 0x11; i >= 0x2; i--) {
		u32 z = keybuf[i] ^ x;
		x = keybuf[0x012 + (z >> 24 & 0xFF)];
		x += keybuf[0x112 + (z >> 16 & 0xFF)];
		x ^= keybuf[0x212 + (z >> 8 & 0xFF)];
		x += keybuf[0x312 + (z & 0xFF)];
		x ^= y;
		y = z;
	}

	writearr<u32>(p, 0, x ^ keybuf[0x1]);
	writearr<u32>(p, 4, y ^ keybuf[0x0]);
}

static void
apply_keycode(cartridge *cart, u32 modulo)
{
	encrypt64(cart->keycode + 4, cart->keybuf);
	encrypt64(cart->keycode, cart->keybuf);
	u8 scratch[8]{};
	for (u32 i = 0; i <= 0x11; i++) {
		cart->keybuf[i] ^= byteswap32(
				readarr<u32>(cart->keycode, i * 4 % modulo));
	}
	for (u32 i = 0; i <= 0x410; i += 2) {
		encrypt64(&scratch[0], cart->keybuf);
		cart->keybuf[i] = readarr<u32>(scratch, 4);
		cart->keybuf[i + 1] = readarr<u32>(scratch, 0);
	}
}

static void
init_keycode(cartridge *cart, u32 gamecode, int level, u32 modulo)
{
	for (u32 i = 0; i < 0x412; i++) {
		u32 v = readarr<u32>(cart->arm7_bios, 0x30 + i * 4);
		cart->keybuf[i] = v;
	}

	writearr<u32>(cart->keycode, 0, gamecode);
	writearr<u32>(cart->keycode, 4, gamecode / 2);
	writearr<u32>(cart->keycode, 8, gamecode * 2);
	if (level >= 1)
		apply_keycode(cart, modulo);
	if (level >= 2)
		apply_keycode(cart, modulo);
	writearr<u32>(cart->keycode, 4, readarr<u32>(cart->keycode, 4) * 2);
	writearr<u32>(cart->keycode, 8, readarr<u32>(cart->keycode, 8) / 2);
	if (level >= 3)
		apply_keycode(cart, modulo);
}

void
encrypt_secure_area(cartridge *cart)
{
	if (cart->size < 0x8000) {
		return;
	}
	u64 id = readarr<u64>(cart->data, 0x4000);
	if (id != 0xE7FFDEFFE7FFDEFF) {
		return;
	}

	init_keycode(cart, cart->gamecode, 3, 8);
	std::memcpy(cart->data + 0x4000, "encryObj", 8);
	for (u32 i = 0; i < 0x800; i += 8) {
		encrypt64(cart->data + 0x4000 + i, cart->keybuf);
	}

	init_keycode(cart, cart->gamecode, 2, 8);
	encrypt64(cart->data + 0x4000, cart->keybuf);
}

cartridge_backup::cartridge_backup(u8 *data, size_t size, int savetype)
	: data(data),
	  size(size),
	  savetype(savetype)
{
}

cartridge::cartridge(u8 *data, size_t size, u8 *save_data, size_t save_size,
		int savetype, u8 *arm7_bios)
	: data(data),
	  size(size),
	  read_mask(std::bit_ceil(size) - 1),
	  chip_id(cartridge_make_chip_id(size)),
	  backup(save_data, save_size, savetype),
	  arm7_bios(arm7_bios)
{
	if (data) {
		gamecode = readarr<u32>(data, 0xC);
		init_keycode(this, gamecode, 1, 8);
	}

	backup.status = 0;

	switch (savetype) {
	case SAVETYPE_EEPROM_512B:
		backup.status = 0xF0;
		break;
	case SAVETYPE_FLASH_256K:
		backup.jedec_id = 0x204012;
		break;
	case SAVETYPE_FLASH_512K:
		backup.jedec_id = 0x204013;
		break;
	case SAVETYPE_FLASH_1M:
		backup.jedec_id = 0x204014;
		break;
	case SAVETYPE_FLASH_8M:
		backup.jedec_id = 0x204015;
		break;
	}

	if ((gamecode & 0xFF) == 'I') {
		backup.infrared = true;
		LOG("deteced infrared cart\n");
	}
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

	if (t.key1) {
		switch (t.command[7] >> 4) {
		case 0x1:
			t.bus_data_r = cart.chip_id;
			break;
		case 0x2:
		{
			u32 offset = (t.start_addr & ~0xFFF) |
			             ((t.start_addr + t.bytes_read) & 0xFFF);
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
					t.bytes_read & 0xFFF, cart.size, -1);
			break;
		case 0xB7:
		{
			u32 offset = (t.start_addr & ~0xFFF) |
			             ((t.start_addr + t.bytes_read) & 0xFFF);
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

	for (u32 i = 0; i < 8; i++) {
		t.command[i] = nds->cart_command_out[7 - i];
	}
	if (t.key1) {
		decrypt64(t.command, cart.keybuf);
	}

	u32 bs_bits = nds->romctrl >> 24 & 7;
	if (bs_bits == 0) {
		t.transfer_size = 0;
	} else if (bs_bits < 7) {
		t.transfer_size = 0x100 << bs_bits;
	} else {
		t.transfer_size = 4;
	}

	t.bytes_read = 0;

	if (t.key1) {
		switch (t.command[7] >> 4) {
		case 0x4:
		case 0x1:
			break;
		case 0xA:
			t.key1 = false;
			break;
		case 0x2:
			t.start_addr = t.command[5] >> 4 & 7;
			t.start_addr <<= 12;
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
			init_keycode(&cart, cart.gamecode, 2, 8);
			break;
		case 0xB7:
			t.start_addr = (u32)t.command[6] << 24 |
			               (u32)t.command[5] << 16 |
			               (u32)t.command[4] << 8 | t.command[3];
			t.start_addr &= cart.read_mask;
			if (t.start_addr < 0x8000) {
				t.start_addr = 0x8000 + (t.start_addr & 0x1FF);
			}
			break;
		case 0xB8:
			break;
		default:
			LOG("unhandled command %016lX\n",
					readarr<u64>(t.command, 0));
		}
	}

	u32 cycles_per_byte = nds->romctrl & BIT(27) ? 8 : 5;

	schedule_nds_event_after(nds, cpuid,
			event_scheduler::ROM_ADVANCE_TRANSFER,
			cycles_per_byte * 8);

	nds->romctrl |= BIT(31);
	nds->romctrl &= ~BIT(23);
}

void
romctrl_write(nds_ctx *nds, int cpuid, u32 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	bool old_start = nds->romctrl & BIT(31);
	bool new_start = value & BIT(31);
	nds->romctrl = (nds->romctrl & (BIT(23) | BIT(29))) |
	               (value & ~(BIT(23) | BIT(31)));

	if (!(!old_start && new_start))
		return;
	if (!(nds->auxspicnt & BIT(15)))
		return;
	if (nds->auxspicnt & BIT(13))
		return;

	cartridge_start_command(nds, cpuid);
}

u32
read_cart_bus_data(nds_ctx *nds, int cpuid)
{
	if (cpuid != nds->nds_slot_cpu)
		return 0;

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

void
auxspicnt_write_l(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	nds->auxspicnt = (nds->auxspicnt & ~0x43) | (value & 0x43);
}

static void
reset_auxspi(nds_ctx *nds)
{
	nds->cart.backup.cs_active = false;
}

void
auxspicnt_write_h(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	bool old_enabled = nds->auxspicnt & BIT(15);
	bool new_enabled = value & BIT(7);
	nds->auxspicnt = (nds->auxspicnt & ~0xE000) |
	                 ((u16)value << 8 & 0xE000);
	if (!old_enabled && new_enabled) {
		reset_auxspi(nds);
	}
}

void
auxspicnt_write(nds_ctx *nds, int cpuid, u16 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	bool old_enabled = nds->auxspicnt & BIT(15);
	bool new_enabled = value & BIT(15);
	nds->auxspicnt = (nds->auxspicnt & ~0xE043) | (value & 0xE043);
	if (!old_enabled && new_enabled) {
		reset_auxspi(nds);
	}
}

static void
eeprom_common_transfer_byte(nds_ctx *nds, u8 value)
{
	auto& bk = nds->cart.backup;

	switch (bk.command) {
	case 0x6:
		bk.status |= BIT(1);
		break;
	case 0x4:
		bk.status &= ~BIT(1);
		break;
	case 0x5:
		if (bk.count > 0) {
			nds->auxspidata_r = bk.status;
		}
		break;
	case 0x1:
		if (bk.count == 1) {
			bk.status = (bk.status & ~0xC) | (value & 0xC);
		}
		break;
	case 0x9F:
		if (bk.count > 0) {
			nds->auxspidata_r = -1;
		}
		break;
	default:
		LOG("unknown eeprom command %02X\n", bk.command);
	}
}

static void
eeprom_512b_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& bk = nds->cart.backup;

	if (!bk.cs_active) {
		bk.command = value;
		bk.count = 0;
	}

	switch (bk.command) {
	case 0x3:
	case 0xB:
		switch (bk.count) {
		case 0:
			break;
		case 1:
			bk.addr = value;
			if (bk.command == 0xB) {
				bk.addr |= 0x100;
			}
			break;
		default:
			nds->auxspidata_r = readarr_checked<u8>(
					bk.data, bk.addr, bk.size, -1);
			bk.addr++;
		}
		break;
	case 0x2:
	case 0xA:
		switch (bk.count) {
		case 0:
			break;
		case 1:
			bk.addr = value;
			if (bk.command == 0xA) {
				bk.addr |= 0x100;
			}
			break;
		default:
			writearr_checked<u8>(bk.data, bk.addr, value, bk.size);
			bk.addr++;
		}
		break;
	default:
		eeprom_common_transfer_byte(nds, value);
	}

	bk.count++;
	bk.cs_active = keep_active;
}

static void
eeprom_8k_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& bk = nds->cart.backup;

	if (!bk.cs_active) {
		bk.command = value;
		bk.count = 0;
	}

	switch (bk.command) {
	case 0x3:
		switch (bk.count) {
		case 0:
			break;
		case 1:
			bk.addr = (u16)value << 8;
			break;
		case 2:
			bk.addr |= value;
			break;
		default:
			nds->auxspidata_r = readarr_checked<u8>(
					bk.data, bk.addr, bk.size, -1);
			bk.addr++;
		}
		break;
	case 0x2:
		switch (bk.count) {
		case 0:
			break;
		case 1:
			bk.addr = (u16)value << 8;
			break;
		case 2:
			bk.addr |= value;
			break;
		default:
			writearr_checked<u8>(bk.data, bk.addr, value, bk.size);
			bk.addr++;
		}
		break;
	default:
		eeprom_common_transfer_byte(nds, value);
	}

	bk.count++;
	bk.cs_active = keep_active;
}

static void
flash_common_transfer_byte(nds_ctx *nds, u8 value)
{
	auto& bk = nds->cart.backup;

	switch (bk.command) {
	case 0x0:
		break;
	case 0x6:
		bk.status |= BIT(1);
		break;
	case 0x4:
		bk.status &= ~BIT(0);
		break;
	case 0x9F:
		switch (bk.count) {
		case 0:
			break;
		case 1:
			nds->auxspidata_r = bk.jedec_id >> 16 & 0xFF;
			break;
		case 2:
			nds->auxspidata_r = bk.jedec_id >> 8 & 0xFF;
			break;
		case 3:
			nds->auxspidata_r = bk.jedec_id & 0xFF;
			break;
		}
		break;
	case 0x5:
		if (bk.count > 0) {
			nds->auxspidata_r = bk.status;
		}
		break;
	case 0x3:
	case 0xB:
		switch (bk.count) {
		case 0:
			break;
		case 1:
			bk.addr = (u32)value << 16;
			break;
		case 2:
			bk.addr |= (u32)value << 8;
			break;
		case 3:
			bk.addr |= value;
			break;
		case 4:
			if (bk.command == 0xB) {
				break;
			}
			[[fallthrough]];
		default:
			nds->auxspidata_r = readarr_checked<u8>(
					bk.data, bk.addr, bk.size, 0);
			bk.addr++;
		}
		break;
	case 0xA:
		switch (bk.count) {
		case 0:
			break;
		case 1:
			bk.addr = (u32)value << 16;
			break;
		case 2:
			bk.addr |= (u32)value << 8;
			break;
		case 3:
			bk.addr |= value;
			break;
		default:
			writearr_checked<u8>(bk.data, bk.addr, value, bk.size);
			bk.addr++;
		}
		break;
	default:
		LOG("unknown flash command %02X\n", bk.command);
		throw twice_error("unknown flash command");
	}
}

static void
flash_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& bk = nds->cart.backup;

	if (!bk.cs_active) {
		bk.command = value;
		bk.count = 0;
	}

	flash_common_transfer_byte(nds, value);

	bk.count++;
	bk.cs_active = keep_active;
}

static void
infrared_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& bk = nds->cart.backup;

	if (!bk.cs_active) {
		bk.ir_command = value;
		bk.ir_count = 0;
	}

	switch (bk.ir_command) {
	case 0x0:
		if (bk.ir_count == 1) {
			bk.command = value;
			bk.count = 0;
		}
		if (bk.ir_count > 0) {
			flash_common_transfer_byte(nds, value);
			bk.count++;
		}
		break;
	case 0x8:
		if (bk.ir_count > 0) {
			nds->auxspidata_r = 0xAA;
		}
		break;
	default:
		LOG("unhandled IR command %02X\n", bk.ir_command);
	}

	bk.ir_count++;
	bk.cs_active = keep_active;
}

void
auxspidata_write(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;
	if (!(nds->auxspicnt & BIT(15)))
		return;
	if (!(nds->auxspicnt & BIT(13)))
		return;

	nds->auxspidata_w = value;
	bool keep_active = nds->auxspicnt & BIT(6);

	switch (nds->cart.backup.savetype) {
	case SAVETYPE_EEPROM_512B:
		eeprom_512b_transfer_byte(nds, value, keep_active);
		break;
	case SAVETYPE_EEPROM_8K:
	case SAVETYPE_EEPROM_64K:
		eeprom_8k_transfer_byte(nds, value, keep_active);
		break;
	case SAVETYPE_FLASH_256K:
	case SAVETYPE_FLASH_512K:
	case SAVETYPE_FLASH_1M:
	case SAVETYPE_FLASH_8M:
		if (nds->cart.backup.infrared) {
			infrared_transfer_byte(nds, value, keep_active);
		} else {
			flash_transfer_byte(nds, value, keep_active);
		}
		break;
	}

	schedule_nds_event_after(nds, cpuid,
			event_scheduler::AUXSPI_TRANSFER_COMPLETE,
			64 << (nds->auxspicnt & 3));
	nds->auxspicnt |= BIT(7);
}

void
event_auxspi_transfer_complete(nds_ctx *nds)
{
	nds->auxspicnt &= ~BIT(7);
}

} // namespace twice
