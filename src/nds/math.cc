#include "nds/math.h"
#include "nds/nds.h"

#include <cfloat>
#include <cmath>

namespace twice {

static u32
sqrt64(u64 n)
{
#if LDBL_MANT_DIG >= 64
	return std::sqrt((long double)n);
#endif
}

static u32
sqrt32(u32 n)
{
	return std::sqrt((double)n);
}

void
nds_math_sqrt(NDS *nds)
{
	if (nds->sqrtcnt & 1) {
		u64 n = (u64)nds->sqrt_param[1] << 32 | nds->sqrt_param[0];
		nds->sqrt_result = sqrt64(n);
	} else {
		nds->sqrt_result = sqrt32(nds->sqrt_param[0]);
	}
}

} // namespace twice
