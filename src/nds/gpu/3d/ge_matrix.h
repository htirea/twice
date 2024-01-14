#ifndef TWICE_GE_MATRIX_H
#define TWICE_GE_MATRIX_H

#include "common/types.h"

namespace twice {

/* matrices are in stored column major */

using ge_matrix = std::span<s32, 16>;
using ge_vector = std::span<s32, 4>;
using ge_vector3 = std::span<s32, 3>;
using const_ge_matrix = std::span<const s32, 16>;
using const_ge_vector = std::span<const s32, 4>;
using const_ge_vector3 = std::span<const s32, 3>;
using ge_params = std::span<const u32, 32>;

void ge_mtx_mult_mtx(ge_matrix r, const_ge_matrix s, const_ge_matrix t);
void ge_mtx_mult_vec(ge_vector r, const_ge_matrix s, const_ge_vector t);
void ge_mtx_mult_vec3(ge_vector3 r, const_ge_matrix s, s32 t0, s32 t1, s32 t2);
void ge_mtx_set_identity(ge_matrix r);
void ge_mtx_load_4x4(ge_matrix r, ge_params t);
void ge_mtx_load_4x3(ge_matrix r, ge_params t);
void ge_mtx_mult_4x4(ge_matrix r, ge_params t);
void ge_mtx_mult_4x3(ge_matrix r, ge_params t);
void ge_mtx_mult_3x3(ge_matrix r, ge_params t);
void ge_mtx_scale(ge_matrix r, ge_params t);
void ge_mtx_trans(ge_matrix r, ge_params t);

} // namespace twice

#endif
