#include "nds/gpu/3d/matrix.h"

namespace twice {

void
mtx_mult_mtx(ge_matrix *r, ge_matrix *s, ge_matrix *t)
{
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 4; k++) {
				sum += (s64)(t->v[i][k]) * s->v[k][j];
			}
			r->v[i][j] = sum >> 12;
		}
	}
}

void
mtx_set_identity(ge_matrix *r)
{
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			r->v[i][j] = i == j ? 1 << 12 : 0;
		}
	}
}

void
mtx_load_4x4(ge_matrix *r, u32 *p)
{
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			r->v[i][j] = *p++;
		}
	}
}

void
mtx_load_4x3(ge_matrix *r, u32 *p)
{
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 3; j++) {
			r->v[i][j] = *p++;
		}
	}

	r->v[0][3] = 0;
	r->v[1][3] = 0;
	r->v[2][3] = 0;
	r->v[3][3] = 1 << 12;
}

void
mtx_mult_4x4(ge_matrix *r, u32 *p)
{
	ge_matrix s = *r;

	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 4; k++) {
				sum += (s64)(s32)p[4 * i + k] * s.v[k][j];
			}
			r->v[i][j] = sum >> 12;
		}
	}
}

void
mtx_mult_4x3(ge_matrix *r, u32 *p)
{
	ge_matrix s = *r;

	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 3; k++) {
				sum += (s64)(s32)p[3 * i + k] * s.v[k][j];
			}
			r->v[i][j] = sum >> 12;
		}
	}

	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 3; k++) {
			sum += (s64)(s32)p[9 + k] * s.v[k][j];
		}
		sum += (s64)s.v[3][j] << 12;
		r->v[3][j] = sum >> 12;
	}
}

void
mtx_mult_3x3(ge_matrix *r, u32 *p)
{
	ge_matrix s = *r;

	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 3; k++) {
				sum += (s64)(s32)p[3 * i + k] * s.v[k][j];
			}
			r->v[i][j] = sum >> 12;
		}
	}

	for (u32 j = 0; j < 4; j++) {
		r->v[3][j] = s.v[3][j];
	}
}

void
mtx_scale(ge_matrix *r, u32 *p)
{
	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			r->v[i][j] = (s64)(s32)p[i] * r->v[i][j] >> 12;
		}
	}
}

void
mtx_trans(ge_matrix *r, u32 *p)
{
	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 3; k++) {
			sum += (s64)(s32)p[k] * r->v[k][j];
		}
		sum += (s64)r->v[3][j] << 12;
		r->v[3][j] = sum >> 12;
	}
}

} // namespace twice
