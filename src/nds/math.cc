#include "nds/math.h"
#include "nds/nds.h"

#include <cfloat>
#include <cmath>

namespace twice {

static u32 sqrt64(u64 n);
static u32 sqrt32(u32 n);

void
nds_math_sqrt(nds_ctx *nds)
{
	if (nds->sqrtcnt & 1) {
		u64 n = (u64)nds->sqrt_param[1] << 32 | nds->sqrt_param[0];
		nds->sqrt_result = sqrt64(n);
	} else {
		nds->sqrt_result = sqrt32(nds->sqrt_param[0]);
	}
}

static u32
sqrt64(u64 n)
{
#if LDBL_MANT_DIG >= 64
	return std::sqrt((long double)n);
#else
	u64 r = 0;
	u64 r2 = 0;

	for (int i = 32; i--;) {
		u64 t = r2 + (r << (i + 1)) + ((u64)1 << (i << 1));
		if (t <= n) {
			r |= (u64)1 << i;
			r2 = t;
		}
	}

	return r;
#endif
}

static u32
sqrt32(u32 n)
{
	return std::sqrt((double)n);
}

static void div32(nds_ctx *nds);
static void div64(nds_ctx *nds, bool denom_is_64_bits);

void
nds_math_div(nds_ctx *nds)
{
	switch (nds->divcnt & 0x3) {
	case 0:
		div32(nds);
		break;
	case 1:
	case 3:
		div64(nds, false);
		break;
	case 2:
		div64(nds, true);
		break;
	}

	if (nds->div_denom[0] == 0 && nds->div_denom[1] == 0) {
		nds->divcnt |= BIT(14);
	} else {
		nds->divcnt &= ~BIT(14);
	}
}

static void
div32(nds_ctx *nds)
{
	s32 numer = nds->div_numer[0];
	s32 denom = nds->div_denom[0];
	s64 rem;

	if (denom == 0) {
		nds->div_result[0] = numer < 0 ? 1 : -1;
		nds->div_result[1] = numer < 0 ? -1 : 0;
		rem = numer;
	} else if ((u32)numer == 0x80000000 && denom == -1) {
		nds->div_result[0] = 0x80000000;
		nds->div_result[1] = 0;
		rem = 0;
	} else {
		s64 result = numer / denom;
		rem = numer % denom;
		nds->div_result[0] = result;
		nds->div_result[1] = result >> 32;
	}

	nds->divrem_result[0] = rem;
	nds->divrem_result[1] = rem >> 32;
}

static void
div64(nds_ctx *nds, bool denom_is_64_bits)
{
	s64 numer = (u64)nds->div_numer[1] << 32 | nds->div_numer[0];
	s64 denom;
	s64 result;
	s64 rem;

	if (denom_is_64_bits) {
		denom = (u64)nds->div_denom[1] << 32 | nds->div_denom[0];
	} else {
		denom = (s32)nds->div_denom[0];
	}

	if (denom == 0) {
		result = numer < 0 ? 1 : -1;
		rem = numer;
	} else if ((u64)numer == BIT(63) && denom == -1) {
		result = BIT(63);
		rem = 0;
	} else {
		result = numer / denom;
		rem = numer % denom;
	}

	nds->div_result[0] = result;
	nds->div_result[1] = result >> 32;
	nds->divrem_result[0] = rem;
	nds->divrem_result[1] = rem >> 32;
}

} // namespace twice
