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

static const char *
savetype_to_str(int savetype)
{
	switch (savetype) {
	case SAVETYPE_UNKNOWN:
		return "unknown";
	case SAVETYPE_NONE:
		return "none";
	case SAVETYPE_EEPROM_512B:
		return "eeprom 512B";
	case SAVETYPE_EEPROM_8K:
		return "eeprom 8K";
	case SAVETYPE_EEPROM_64K:
		return "eeprom 64K";
	case SAVETYPE_EEPROM_128K:
		return "eeprom 128K";
	case SAVETYPE_FLASH_256K:
		return "flash 256K";
	case SAVETYPE_FLASH_512K:
		return "flash 512K";
	case SAVETYPE_FLASH_1M:
		return "flash 1M";
	case SAVETYPE_FLASH_8M:
		return "flash 8M";
	case SAVETYPE_NAND:
		return "nand";
	default:
		return "invalid";
	}
}

static size_t
savetype_to_size(int savetype)
{
	switch (savetype) {
	case SAVETYPE_UNKNOWN:
		throw twice_error("unknown size");
	case SAVETYPE_NONE:
		return 0;
	case SAVETYPE_EEPROM_512B:
		return 512;
	case SAVETYPE_EEPROM_8K:
		return 8_KiB;
	case SAVETYPE_EEPROM_64K:
		return 64_KiB;
	case SAVETYPE_EEPROM_128K:
		return 128_KiB;
	case SAVETYPE_FLASH_256K:
		return 256_KiB;
	case SAVETYPE_FLASH_512K:
		return 512_KiB;
	case SAVETYPE_FLASH_1M:
		return 1_MiB;
	case SAVETYPE_FLASH_8M:
		return 8_MiB;
	case SAVETYPE_NAND:
		throw twice_error("nand save is unsupported");
	default:
		throw twice_error("invalid savetype");
	}
}

static int
lookup_savetype(u32 gamecode)
{
	if (gamecode == 0 || gamecode == 0x23232323) {
		LOG("detected homebrew: assuming save type: none\n");
		return SAVETYPE_NONE;
	}

	auto it = std::lower_bound(game_db.begin(), game_db.end(), gamecode,
			[](const auto& entry, const auto& gamecode) {
				return entry.gamecode < gamecode;
			});
	if (it == game_db.end() || gamecode < it->gamecode) {
		LOG("unknown save type: gamecode %08X\n", gamecode);
		return SAVETYPE_UNKNOWN;
	}

	LOG("detected save type: %s\n", savetype_to_str(it->savetype));
	return it->savetype;
}

int
nds_get_savetype(u8 *data)
{
	u32 gamecode = readarr<u32>(data, 0xC);
	return lookup_savetype(gamecode);
}

size_t
nds_get_savefile_size(int savetype)
{
	return savetype_to_size(savetype);
}

cartridge::cartridge(u8 *data, size_t size, u8 *save_data, size_t save_size,
		int savetype)
	: data(data),
	  size(size),
	  read_mask(std::bit_ceil(size) - 1),
	  chip_id(cartridge_make_chip_id(size)),
	  save_data(save_data),
	  save_size(save_size),
	  savetype(savetype)
{
	if (data) {
		gamecode = readarr<u32>(data, 0xC);
	}

	switch (savetype) {
	case SAVETYPE_EEPROM_512B:
		backup.status_reg = 0xF0;
		break;
	case SAVETYPE_EEPROM_8K:
	case SAVETYPE_EEPROM_64K:
		backup.status_reg = 0;
		break;
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

void
romctrl_write(nds_ctx *nds, int cpuid, u32 value)
{
	if (cpuid != nds->nds_slot_cpu) return;

	bool old_start = nds->romctrl & BIT(31);
	bool new_start = value & BIT(31);
	nds->romctrl = (nds->romctrl & (BIT(23) | BIT(29))) |
	               (value & ~(BIT(23) | BIT(31)));

	if (!(!old_start && new_start)) return;
	if (!(nds->auxspicnt & BIT(15))) return;
	if (nds->auxspicnt & BIT(13)) return;

	cartridge_start_command(nds, cpuid);
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

void
auxspicnt_write_l(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu) return;

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
	if (cpuid != nds->nds_slot_cpu) return;

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
	if (cpuid != nds->nds_slot_cpu) return;

	bool old_enabled = nds->auxspicnt & BIT(15);
	bool new_enabled = value & BIT(15);
	nds->auxspicnt = (nds->auxspicnt & ~0xE043) | (value & 0xE043);
	if (!old_enabled && new_enabled) {
		reset_auxspi(nds);
	}
}

static void
eeprom_8k_transfer_byte(nds_ctx *nds, u8 value, bool keep_active)
{
	auto& cart = nds->cart;
	auto& bk = nds->cart.backup;

	if (!bk.cs_active) {
		bk.input_bytes.clear();
		bk.command = value;
		bk.count = 0;
	}

	switch (bk.command) {
	case 0x6:
		bk.status_reg |= BIT(1);
		break;
	case 0x4:
		bk.status_reg &= ~BIT(1);
		break;
	case 0x5:
		if (bk.count > 0) {
			nds->auxspidata_r = bk.status_reg;
		}
		break;
	case 0x1:
		if (bk.count == 1) {
			bk.status_reg = (bk.status_reg & ~0xC) | (value & 0xC);
		}
		break;
	case 0x9F:
		if (bk.count > 0) {
			nds->auxspidata_r = -1;
		}
		break;
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
			nds->auxspidata_r = readarr_checked<u8>(cart.save_data,
					bk.addr, cart.save_size, -1);
			bk.addr += 1;
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
			writearr_checked<u8>(cart.save_data, bk.addr, value,
					cart.save_size);
			bk.addr += 1;
		}
		break;
	default:
		LOG("unknown eeprom command %02X\n", bk.command);
		throw twice_error("unknown eeprom command");
	}

	bk.count++;
	bk.cs_active = keep_active;
}

void
auxspidata_write(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu) return;
	if (!(nds->auxspicnt & BIT(15))) return;
	if (!(nds->auxspicnt & BIT(13))) return;

	nds->auxspidata_w = value;

	bool keep_active = nds->auxspicnt & BIT(6);

	switch (nds->cart.savetype) {
	case SAVETYPE_EEPROM_8K:
	case SAVETYPE_EEPROM_64K:
		eeprom_8k_transfer_byte(nds, value, keep_active);
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
