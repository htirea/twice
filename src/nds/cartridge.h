#ifndef TWICE_CARTRIDGE_H
#define TWICE_CARTRIDGE_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

enum cartridge_save_type {
	SAVETYPE_UNKNOWN = -1,
	SAVETYPE_NONE = 0,
	SAVETYPE_EEPROM_512B,
	SAVETYPE_EEPROM_8K,
	SAVETYPE_EEPROM_64K,
	SAVETYPE_EEPROM_128K,
	SAVETYPE_FLASH_256K,
	SAVETYPE_FLASH_512K,
	SAVETYPE_FLASH_1M,
	SAVETYPE_FLASH_8M,
	SAVETYPE_NAND,
};

struct cartridge_db_entry {
	u32 gamecode;
	u32 size;
	int save_type;
};

extern const std::vector<cartridge_db_entry> game_db;

struct cartridge {
	cartridge(u8 *data, size_t size);

	u8 *data{};
	size_t size{};
	size_t read_mask{};
	u32 chip_id{};

	struct rom_transfer {
		u8 command[8]{};
		u32 transfer_size{};
		u32 bytes_read{};
		u32 bus_data_r{};

		u32 start_addr{};
	} transfer;
};

u32 read_cart_bus_data(nds_ctx *nds, int cpuid);
void cartridge_start_command(nds_ctx *nds, int cpuid);
void event_advance_rom_transfer(nds_ctx *nds);
void event_auxspi_transfer_complete(nds_ctx *nds);

void auxspicnt_write_l(nds_ctx *nds, int cpuid, u8 value);
void auxspicnt_write_h(nds_ctx *nds, int cpuid, u8 value);
void auxspicnt_write(nds_ctx *nds, int cpuid, u16 value);
void auxspidata_write(nds_ctx *nds, int cpuid, u8 value);
void romctrl_write(nds_ctx *nds, int cpuid, u32 value);

} // namespace twice

#endif
