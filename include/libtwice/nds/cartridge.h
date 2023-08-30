#ifndef LIBTWICE_NDS_CARTRIDGE_H
#define LIBTWICE_NDS_CARTRIDGE_H

#include "libtwice/filemap.h"
#include "libtwice/nds/game_db.h"

namespace twice {

struct nds_cartridge {
	file_map rom;
	file_map save;
	nds_save_info save_info;
};

} // namespace twice

#endif
