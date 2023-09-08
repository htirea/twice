#ifndef TWICE_MATRIX_H
#define TWICE_MATRIX_H

#include "common/types.h"

namespace twice {

struct ge_matrix {
	/*
	 * LAYOUT:
	 *
	 * v[0][0] v[1][0] v[2][0] v[3][0]
	 * v[0][1] v[1][1] v[2][1] v[3][1]
	 * v[0][2] v[1][2] v[2][2] v[3][2]
	 * v[0][3] v[1][3] v[2][3] v[3][3]
	 */

	s32 v[4][4]{};
};

struct ge_vector {
	s32 v[4]{};
};

void mtx_mult_mtx(ge_matrix *r, ge_matrix *s, ge_matrix *t);
void mtx_mult_vec(ge_vector *r, ge_matrix *m, ge_vector *p);
void mtx_mult_vec3(s32 *r, ge_matrix *m, s32 p0, s32 p1, s32 p2);

void mtx_set_identity(ge_matrix *m);
void mtx_load_4x4(ge_matrix *m, u32 *p);
void mtx_load_4x3(ge_matrix *m, u32 *p);
void mtx_mult_4x4(ge_matrix *r, u32 *p);
void mtx_mult_4x3(ge_matrix *r, u32 *p);
void mtx_mult_3x3(ge_matrix *r, u32 *p);
void mtx_scale(ge_matrix *r, u32 *p);
void mtx_trans(ge_matrix *r, u32 *p);

} // namespace twice

#endif
