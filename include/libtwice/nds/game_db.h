#ifndef LIBTWICE_NDS_GAME_DB_H
#define LIBTWICE_NDS_GAME_DB_H

#include <string>

#include "libtwice/file/file_view.h"
#include "libtwice/types.h"

namespace twice {

enum nds_savetype {
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
	SAVETYPE_DUMMY,
	SAVETYPE_TOTAL,
};

struct nds_save_info {
	nds_savetype type;
	std::streamoff size;
};

struct cartridge_db_entry {
	nds_savetype type;
};

void add_nds_game_db_entry(u32 gamecode, const cartridge_db_entry& entry);
nds_save_info nds_get_save_info(u32 gamecode);
const char *nds_savetype_to_str(nds_savetype type);
std::streamoff nds_savetype_to_size(nds_savetype type);
nds_savetype nds_savetype_from_size(std::streamoff size);
nds_savetype nds_parse_savetype_string(const std::string& s);

} // namespace twice

#endif
