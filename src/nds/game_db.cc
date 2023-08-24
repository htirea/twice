#include "nds/cartridge.h"

#include "common/util.h"

namespace twice {

std::vector<cartridge_db_entry> game_db;

int
nds_initialize_game_db(u8 *db_data, size_t db_size)
{
	if (db_size % 12 != 0) {
		return 1;
	}

	for (size_t idx = 0; idx < db_size; idx += 12) {
		u32 gamecode = readarr<u32>(db_data, idx);
		u32 size = readarr<u32>(db_data, idx + 4);
		s32 savetype = readarr<u32>(db_data, idx + 8);

		game_db.push_back({ gamecode, size, savetype });
	}

	return 0;
}

} // namespace twice
