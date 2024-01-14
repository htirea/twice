#include "nds/gpu/3d/ge_matrix.h"

#include "libtwice/util/matrix.h"

namespace twice {

void
ge_mtx_mult_mtx(ge_matrix r, const_ge_matrix s, const_ge_matrix t)
{
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 4; k++) {
				sum += (s64)s[k * 4 + j] * t[i * 4 + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}
}

void
ge_mtx_mult_vec(ge_vector r, const_ge_matrix s, const_ge_vector t)
{
	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 4; k++) {
			sum += (s64)s[k * 4 + j] * t[k];
		}
		r[j] = sum >> 12;
	}
}

void
ge_mtx_mult_vec3(ge_vector3 r, const_ge_matrix s, s32 t0, s32 t1, s32 t2)
{
	for (u32 j = 0; j < 3; j++) {
		s64 sum = (s64)s[j] * t0 + (s64)s[4 + j] * t1 +
		          (s64)s[8 + j] * t2;
		r[j] = sum >> 12;
	}
}

void
ge_mtx_set_identity(ge_matrix r)
{
	mtx_set_identity<s32, 4>(r, 1 << 12);
}

void
ge_mtx_load_4x4(ge_matrix r, ge_params t)
{
	for (u32 i = 0; i < 16; i++) {
		r[i] = t[i];
	}
}

void
ge_mtx_load_4x3(ge_matrix r, ge_params ts)
{
	auto t = ts.data();

	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 3; j++) {
			r[i * 4 + j] = *t++;
		}
	}

	r[3] = 0;
	r[7] = 0;
	r[11] = 0;
	r[15] = 1 << 12;
}

void
ge_mtx_mult_4x4(ge_matrix r, ge_params t)
{
	std::array<s32, r.size()> s;
	std::ranges::copy(r, s.begin());

	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 4; k++) {
				sum += (s64)s[k * 4 + j] * (s32)t[4 * i + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}
}

void
ge_mtx_mult_4x3(ge_matrix r, ge_params t)
{
	std::array<s32, r.size()> s;
	std::ranges::copy(r, s.begin());

	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 3; k++) {
				sum += (s64)s[k * 4 + j] * (s32)t[3 * i + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}

	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 3; k++) {
			sum += (s64)s[k * 4 + j] * (s32)t[9 + k];
		}
		sum += (s64)s[12 + j] << 12;
		r[12 + j] = sum >> 12;
	}
}

void
ge_mtx_mult_3x3(ge_matrix r, ge_params t)
{
	std::array<s32, r.size()> s;
	std::ranges::copy(r, s.begin());

	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			s64 sum = 0;
			for (u32 k = 0; k < 3; k++) {
				sum += (s64)s[k * 4 + j] * (s32)t[3 * i + k];
			}
			r[i * 4 + j] = sum >> 12;
		}
	}

	for (u32 j = 0; j < 4; j++) {
		r[12 + j] = s[12 + j];
	}
}

void
ge_mtx_scale(ge_matrix r, ge_params t)
{
	for (u32 i = 0; i < 3; i++) {
		for (u32 j = 0; j < 4; j++) {
			r[i * 4 + j] = (s64)r[i * 4 + j] * (s32)t[i] >> 12;
		}
	}
}

void
ge_mtx_trans(ge_matrix r, ge_params t)
{
	for (u32 j = 0; j < 4; j++) {
		s64 sum = 0;
		for (u32 k = 0; k < 3; k++) {
			sum += (s64)r[k * 4 + j] * (s32)t[k];
		}
		sum += (s64)r[12 + j] << 12;
		r[12 + j] = sum >> 12;
	}
}

} // namespace twice
