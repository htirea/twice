#include "nds/cart/backup.h"
#include "nds/nds.h"

#include "libtwice/nds/game_db.h"

namespace twice {

static void reset_auxspi(nds_ctx *nds);
static void eeprom_512b_transfer_byte(nds_ctx *, u8 value);
static void eeprom_8k_transfer_byte(nds_ctx *, u8 value);
static void eeprom_common_transfer_byte(nds_ctx *, u8 value);
static void flash_transfer_byte(nds_ctx *, u8 value);
static void infrared_transfer_byte(nds_ctx *, u8 value);
static void flash_common_transfer_byte(nds_ctx *, u8 value);

void
cartridge_backup_init(nds_ctx *nds, int savetype)
{
	auto& bk = nds->cart.backup;
	bk.data = nds->save_v.data();
	bk.size = nds->save_v.size();
	bk.savetype = savetype;

	switch (savetype) {
	case SAVETYPE_EEPROM_512B:
		bk.stat_reg = 0xF0;
		break;
	case SAVETYPE_FLASH_256K:
		bk.jedec_id = 0x204012;
		break;
	case SAVETYPE_FLASH_512K:
		bk.jedec_id = 0x204013;
		break;
	case SAVETYPE_FLASH_1M:
		bk.jedec_id = 0x204014;
		break;
	case SAVETYPE_FLASH_8M:
		bk.jedec_id = 0x204015;
		break;
	}
}

void
auxspicnt_write_l(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	nds->auxspicnt = (nds->auxspicnt & ~0x43) | (value & 0x43);
}

void
auxspicnt_write_h(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	bool old_enabled = nds->auxspicnt & BIT(15);
	bool enabled = value & BIT(7);

	nds->auxspicnt = (nds->auxspicnt & ~0xE000) |
	                 ((u16)value << 8 & 0xE000);
	if (!old_enabled && enabled) {
		reset_auxspi(nds);
	}
}

void
auxspicnt_write(nds_ctx *nds, int cpuid, u16 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	bool old_enabled = nds->auxspicnt & BIT(15);
	bool enabled = value & BIT(15);

	nds->auxspicnt = (nds->auxspicnt & ~0xE043) | (value & 0xE043);
	if (!old_enabled && enabled) {
		reset_auxspi(nds);
	}
}

void
auxspidata_write(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu)
		return;

	auto& bk = nds->cart.backup;
	bool enabled = nds->auxspicnt & BIT(15);
	bool serial_mode = nds->auxspicnt & BIT(13);
	bool keep_active = nds->auxspicnt & BIT(6);

	if (!enabled || !serial_mode)
		return;

	switch (bk.savetype) {
	case SAVETYPE_EEPROM_512B:
		eeprom_512b_transfer_byte(nds, value);
		break;
	case SAVETYPE_EEPROM_8K:
	case SAVETYPE_EEPROM_64K:
		eeprom_8k_transfer_byte(nds, value);
		break;
	case SAVETYPE_FLASH_256K:
	case SAVETYPE_FLASH_512K:
	case SAVETYPE_FLASH_1M:
	case SAVETYPE_FLASH_8M:
		if (nds->cart.infrared) {
			infrared_transfer_byte(nds, value);
		} else {
			flash_transfer_byte(nds, value);
		}
		break;
	}

	bk.cs_active = keep_active;
	schedule_event_after(nds, cpuid, scheduler::AUXSPI_TRANSFER,
			64 << (nds->auxspicnt & 3) << 1);
	nds->auxspicnt |= BIT(7);
}

void
event_auxspi_transfer_complete(nds_ctx *nds, intptr_t, timestamp)
{
	nds->auxspicnt &= ~BIT(7);
}

static void
reset_auxspi(nds_ctx *nds)
{
	nds->cart.backup.cs_active = false;
}

static void
eeprom_512b_transfer_byte(nds_ctx *nds, u8 value)
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
}

static void
eeprom_8k_transfer_byte(nds_ctx *nds, u8 value)
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
}

static void
eeprom_common_transfer_byte(nds_ctx *nds, u8 value)
{
	auto& bk = nds->cart.backup;

	switch (bk.command) {
	case 0x6:
		bk.stat_reg |= BIT(1);
		break;
	case 0x4:
		bk.stat_reg &= ~BIT(1);
		break;
	case 0x5:
		if (bk.count > 0) {
			nds->auxspidata_r = bk.stat_reg;
		}
		break;
	case 0x1:
		if (bk.count == 1) {
			bk.stat_reg = (bk.stat_reg & ~0xC) | (value & 0xC);
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
flash_transfer_byte(nds_ctx *nds, u8 value)
{
	auto& bk = nds->cart.backup;

	if (!bk.cs_active) {
		bk.command = value;
		bk.count = 0;
	}

	flash_common_transfer_byte(nds, value);

	bk.count++;
}

static void
infrared_transfer_byte(nds_ctx *nds, u8 value)
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
}

static void
flash_common_transfer_byte(nds_ctx *nds, u8 value)
{
	auto& bk = nds->cart.backup;

	switch (bk.command) {
	case 0x0:
		break;
	case 0x6:
		bk.stat_reg |= BIT(1);
		break;
	case 0x4:
		bk.stat_reg &= ~BIT(0);
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
			nds->auxspidata_r = bk.stat_reg;
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

} // namespace twice
