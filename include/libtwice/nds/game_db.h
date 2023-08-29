#ifndef LIBTWICE_NDS_GAME_DB_H
#define LIBTWICE_NDS_GAME_DB_H

#include <string>

#include "libtwice/filemap.h"
#include "libtwice/types.h"

namespace twice {

enum nds_savetype : s32 {
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

struct nds_save_info {
	nds_savetype type;
	size_t size;
};

void nds_load_game_db(const std::string& pathname);
nds_save_info nds_get_save_info(const file_map& cartridge);

} // namespace twice

#endif
