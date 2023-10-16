#ifndef TWICE_SDL_DATABASE_H
#define TWICE_SDL_DATABASE_H

#include <string>

#include "libtwice/exception.h"

namespace twice {

struct database_error : twice_exception {
	using twice_exception::twice_exception;
};

void initialize_nds_game_db_from_json(const std::string& filename);

} // namespace twice

#endif
