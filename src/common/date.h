#ifndef TWICE_COMMON_DATE_H
#define TWICE_COMMON_DATE_H

#include "common/types.h"

namespace twice {

bool is_leap_year(int year);
int get_days_in_month(int month, int year);

} // namespace twice

#endif
