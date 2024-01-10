#include "libtwice/nds/game_db.h"
#include "libtwice/filemap.h"

#include "common/logger.h"
#include "common/util.h"

#include <set>

namespace twice {

static std::map<u32, cartridge_db_entry> game_db;

void
add_nds_game_db_entry(u32 gamecode, const cartridge_db_entry& entry)
{
	game_db[gamecode] = entry;
}

static nds_savetype lookup_savetype(u32 gamecode);

nds_save_info
nds_get_save_info(const file_map& cartridge)
{
	u32 gamecode = readarr<u32>(cartridge.data(), 0xC);
	nds_savetype type = lookup_savetype(gamecode);
	size_t size = nds_savetype_to_size(type);

	return { type, size };
}

static nds_savetype
lookup_savetype(u32 gamecode)
{
	if (gamecode == 0 || gamecode == 0x23232323) {
		LOG("detected homebrew: assuming save type: none\n");
		return SAVETYPE_NONE;
	}

	auto it = game_db.find(gamecode);
	if (it == game_db.end()) {
		LOG("unknown save type: gamecode %08X %c%c%c%c\n", gamecode,
				gamecode & 0xFF, gamecode >> 8 & 0xFF,
				gamecode >> 16 & 0xFF, gamecode >> 24 & 0xFF);
		return SAVETYPE_UNKNOWN;
	}

	LOG("detected save type: %s\n", nds_savetype_to_str(it->second.type));
	return it->second.type;
}

const char *
nds_savetype_to_str(nds_savetype type)
{
	switch (type) {
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

size_t
nds_savetype_to_size(nds_savetype type)
{
	switch (type) {
	case SAVETYPE_UNKNOWN:
		return 0;
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

nds_savetype
nds_savetype_from_size(size_t size)
{
	switch (size) {
	case 8_MiB:
		return SAVETYPE_FLASH_8M;
	case 1_MiB:
		return SAVETYPE_FLASH_1M;
	case 512_KiB:
		return SAVETYPE_FLASH_512K;
	case 256_KiB:
		return SAVETYPE_FLASH_256K;
	case 128_KiB:
		return SAVETYPE_EEPROM_128K;
	case 64_KiB:
		return SAVETYPE_EEPROM_64K;
	case 8_KiB:
		return SAVETYPE_EEPROM_8K;
	case 512:
		return SAVETYPE_EEPROM_512B;
	default:
		return SAVETYPE_NONE;
	}
}

static bool
contains(const std::string& s, const std::string& substr)
{
	return s.find(substr) != s.npos;
}

nds_savetype
nds_parse_savetype_string(const std::string& s)
{
	if (s.length() == 0)
		return SAVETYPE_NONE;

	if (s == "unknown")
		return SAVETYPE_UNKNOWN;

	if (s == "none")
		return SAVETYPE_NONE;

	if (s[0] == 'e') {
		if (contains(s, "512"))
			return SAVETYPE_EEPROM_512B;
		else if (contains(s, "128"))
			return SAVETYPE_EEPROM_128K;
		else if (contains(s, "64"))
			return SAVETYPE_EEPROM_64K;
		else if (contains(s, "8"))
			return SAVETYPE_EEPROM_8K;
		else
			throw twice_error("invalid eeprom: " + s);
	}

	if (s[0] == 'f') {
		if (contains(s, "512"))
			return SAVETYPE_FLASH_512K;
		else if (contains(s, "256"))
			return SAVETYPE_FLASH_256K;
		else if (contains(s, "1"))
			return SAVETYPE_FLASH_1M;
		else if (contains(s, "8"))
			return SAVETYPE_FLASH_8M;
		else
			throw twice_error("invalid flash: " + s);
	}

	throw twice_error("could not parse save type from: " + s);
}

} // namespace twice
