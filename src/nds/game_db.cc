#include "nds/cartridge.h"

namespace twice {

const std::vector<cartridge_db_entry> game_db = {
#include "game_db.inc"
};

} // namespace twice
