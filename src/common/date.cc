#include "common/date.h"

namespace twice {

bool
is_leap_year(int year)
{
	if (year % 400 == 0)
		return true;

	if (year % 100 == 0)
		return false;

	return year % 4 == 0;
}

int
get_days_in_month(int month, int year)
{
	switch (month) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return 31;
	case 2:
		return is_leap_year(year) ? 29 : 28;
	default:
		return 30;
	}
}

} // namespace twice
