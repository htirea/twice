#ifndef LIBTWICE_NDS_GAME_DB_H
#define LIBTWICE_NDS_GAME_DB_H

#include <string>

#include "libtwice/types.h"
#include "libtwice/util/filemap.h"

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

struct cartridge_db_entry {
	nds_savetype type;
};

void add_nds_game_db_entry(u32 gamecode, const cartridge_db_entry& entry);
nds_save_info nds_get_save_info(const file_map& cartridge);
const char *nds_savetype_to_str(nds_savetype type);
size_t nds_savetype_to_size(nds_savetype type);
nds_savetype nds_savetype_from_size(size_t size);
nds_savetype nds_parse_savetype_string(const std::string& s);

} // namespace twice

#endif
