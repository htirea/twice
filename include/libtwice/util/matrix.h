#ifndef LIBTWICE_MATRIX_H
#define LIBTWICE_MATRIX_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace twice {

/**
 * Matrices are stored in column major order.
 */

template <typename T, std::size_t M>
void
mtx_set_identity(std::span<T, M * M> v, T one = 1)
{
	std::ranges::fill(v, 0);

	for (std::size_t m = 0; m < M; m++) {
		v[M * m + m] = one;
	}
}

template <typename R, typename T, std::size_t N>
R
vector_dot(std::span<T, N> s, std::span<T, N> t)
{
	R r = 0;

	for (std::size_t i = 0; i < N; i++) {
		r += static_cast<R>(s[i]) * static_cast<R>(t[i]);
	}

	return r;
}

} // namespace twice

#endif
