#include "libtwice/config.h"

#include "common/logger.h"

namespace twice {

void
set_logger_verbose_level(int level)
{
	logger_verbose_level = level;
}

} // namespace twice
