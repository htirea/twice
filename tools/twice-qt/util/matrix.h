#ifndef TWICE_MATRIX_H
#define TWICE_MATRIX_H

#include <cstddef>
#include <cstdint>

namespace twice {

/**
 * Matrices are stored in column major order.
 */

template <typename T, std::size_t M>
void
mtx_set_identity(T *v, T one = 1)
{
	for (std::size_t m = 0; m < M; m++) {
		v[M * m + m] = one;
	}
}

} // namespace twice

#endif
