#ifndef LIBTWICE_TRIPLE_BUFFER_H
#define LIBTWICE_TRIPLE_BUFFER_H

#include <array>
#include <atomic>
#include <cstdint>

namespace twice {

template <typename T>
struct triple_buffer {
	triple_buffer();

	triple_buffer(const T& v) : _buffers{ v, v, v } {}

	T& get_write_buffer()
	{
		/* write_idx can only modified by the writing thread */
		return _buffers[write_idx & IDX_MASK];
	}

	void swap_write_buffer()
	{
		/* need to swap the shared_idx with the write_idx */
		write_idx = shared_idx.exchange(write_idx | DIRTY_BIT);
	}

	T& get_read_buffer()
	{
		/* if dirty bit set, need to swap */
		if (shared_idx & DIRTY_BIT) {
			read_idx = shared_idx.exchange(read_idx & ~DIRTY_BIT);
		}

		return _buffers[read_idx & IDX_MASK];
	}

      private:
	std::array<T, 3> _buffers;
	std::atomic_size_t write_idx{ 0 };
	std::atomic_size_t shared_idx{ 1 };
	std::atomic_size_t read_idx{ 2 };
	static constexpr std::size_t DIRTY_BIT = 4;
	static constexpr std::size_t IDX_MASK = 3;
};

} // namespace twice

#endif
