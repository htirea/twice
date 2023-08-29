#ifndef TWICE_CARTRIDGE_H
#define TWICE_CARTRIDGE_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

enum {
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
	int savetype;
};

extern std::vector<cartridge_db_entry> game_db;

struct cartridge_backup {
	cartridge_backup(u8 *data, size_t size, int savetype);

	u8 *data{};
	size_t size{};
	int savetype{};

	u8 command{};
	u32 count{};
	u32 addr{};
	u8 status{};
	u32 jedec_id{};
	bool cs_active{};
};

struct cartridge {
	cartridge(u8 *data, size_t size, u8 *save_data, size_t save_size,
			int savetype, u8 *arm7_bios);

	u8 *data{};
	size_t size{};
	size_t read_mask{};
	u32 chip_id{};
	u32 gamecode{};

	struct rom_transfer {
		u8 command[8]{};
		u32 transfer_size{};
		u32 bytes_read{};
		u32 bus_data_r{};

		u32 start_addr{};
		bool key1{};
	} transfer;

	cartridge_backup backup;

	u8 *arm7_bios{};
	u32 keybuf[0x412]{};
	u8 keycode[16]{};
};

void encrypt_secure_area(cartridge *cart);
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
