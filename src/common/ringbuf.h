#ifndef TWICE_RINGBUF
#define TWICE_RINGBUF

#include "common/types.h"

namespace twice {

/*
 * Basic ring buffer queue using a statically backed array
 * Don't use types that are expensive to default allocate
 */
template <typename T, size_t N>
struct ringbuf {
	ringbuf() : read_idx(0), write_idx(0), sz(0) {}

	void push(const T& x)
	{
		if constexpr (power_of_two) {
			buffer[write_idx++ & (N - 1)] = x;
			sz++;
		} else {
			buffer[write_idx++] = x;
			sz++;
			if (write_idx == N) {
				write_idx = 0;
			}
		}
	}

	T& pop()
	{
		if constexpr (power_of_two) {
			sz--;
			return buffer[read_idx++ & (N - 1)];
		} else {
			size_t idx = read_idx++;
			if (read_idx == N) {
				read_idx = 0;
			}
			sz--;
			return buffer[idx];
		}
	}

	T& front()
	{
		if constexpr (power_of_two) {
			return buffer[read_idx & (N - 1)];
		} else {
			return buffer[read_idx];
		}
	}

	void pop(size_t n)
	{
		sz -= n;
		if constexpr (power_of_two) {
			read_idx += n;
		} else {
			read_idx = (read_idx + n) % N;
		}
	}

	T& operator[](size_t i) { return buffer[(read_idx + i) % N]; }

	size_t size() const noexcept { return sz; }

	bool empty() const noexcept { return sz == 0; }

      private:
	static constexpr bool power_of_two = std::has_single_bit(N);
	T buffer[N];
	size_t read_idx;
	size_t write_idx;
	size_t sz;
};

} // namespace twice

#endif
