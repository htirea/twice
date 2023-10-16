#include "database.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "libtwice/nds/game_db.h"

using json = nlohmann::json;

namespace twice {

static nds_savetype
get_nds_savetype(const std::string& s)
{
	if (s == "none")
		return SAVETYPE_NONE;
	else if (s == "eeprom 512B")
		return SAVETYPE_EEPROM_512B;
	else if (s == "eeprom 8K")
		return SAVETYPE_EEPROM_8K;
	else if (s == "eeprom 64K")
		return SAVETYPE_EEPROM_64K;
	else if (s == "eeprom 128K")
		return SAVETYPE_EEPROM_128K;
	else if (s == "flash 256K")
		return SAVETYPE_FLASH_256K;
	else if (s == "flash 512K")
		return SAVETYPE_FLASH_512K;
	else if (s == "flash 1M")
		return SAVETYPE_FLASH_1M;
	else if (s == "flash 8M")
		return SAVETYPE_FLASH_8M;
	else
		throw database_error("unknown save type: " + s);
}

void
initialize_nds_game_db_from_json(const std::string& filename)
try {
	std::ifstream f(filename);
	if (!f) {
		throw database_error("could not open file: " + filename);
	}

	auto data = json::parse(f);
	for (const auto& entry : data) {
		const auto& serial = std::string(entry.at("serial"));
		if (serial.size() != 4) {
			throw database_error("invalid serial: " + serial);
		}

		auto savetype = get_nds_savetype(
				std::string(entry.at("save-type")));

		u32 gamecode = (u32)serial[3] << 24 | (u32)serial[2] << 16 |
		               (u32)serial[1] << 8 | (u32)serial[0];
		add_nds_game_db_entry(gamecode, { savetype });
	}
} catch (const json::exception& e) {
	throw database_error(
			"error while reading json: " + std::string(e.what()));
}

} // namespace twice
