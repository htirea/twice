#ifndef TWICE_SDL_MOVING_AVERAGE_H
#define TWICE_SDL_MOVING_AVERAGE_H

#include "libtwice/types.h"

namespace twice {

template <typename T, size_t N = 64>
class moving_average {
	T buffer[N]{};
	size_t idx{};
	T sum{};

      public:
	void insert(const T& new_value)
	{
		T old_value = buffer[idx];
		sum = sum + new_value - old_value;
		buffer[idx] = new_value;
		idx = (idx + 1) % N;
	}

	T get_average() { return sum / N; }
};

} // namespace twice

#endif
