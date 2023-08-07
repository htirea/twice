#ifndef TWICE_LOGGER_H
#define TWICE_LOGGER_H

#include "common/types.h"

namespace twice {

extern int logger_verbose_level;

[[gnu::format(printf, 1, 2)]] void LOG(const char *format, ...);
[[gnu::format(printf, 1, 2)]] void LOGV(const char *format, ...);
[[gnu::format(printf, 1, 2)]] void LOGVV(const char *format, ...);

} // namespace twice

#endif
