#ifndef TWICE_LOGGER_H
#define TWICE_LOGGER_H

#include "common/types.h"

namespace twice {

extern int logger_verbose_level;

void LOG(const char *format, ...);
void LOGV(const char *format, ...);
void LOGVV(const char *format, ...);

} // namespace twice

#endif
