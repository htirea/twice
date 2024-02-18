#ifndef LIBTWICE_RING_BUFFER_H
#define LIBTWICE_RING_BUFFER_H

#include "libtwice/types.h"

#include <array>
#include <cstring>
#include <mutex>

namespace twice {

template <size_t N>
class ring_buffer {
      public:
	size_t read(char *data, size_t count)
	{
		if (count > size) {
			count = size;
		}

		auto count1 = std::min(count, N - read_idx);
		auto count2 = count - count1;
		auto it = buffer.begin();
		std::copy(it + read_idx, it + read_idx + count1, data);
		std::copy(it, it + count2, data + count1);
		read_idx = (read_idx + count) % N;
		size -= count;

		return count;
	}

	void read_fill_zero(char *data, size_t count)
	{
		auto bytes_read = read(data, count);
		if (bytes_read < count) {
			auto bytes_left = count - bytes_read;
			std::fill(data + bytes_read,
					data + bytes_read + bytes_left, 0);
		}
	}

	size_t write(const char *data, size_t count)
	{
		auto ret = count;

		if (count > N) {
			auto skip = count - N;
			data += skip;
			write_idx = (write_idx + skip) % N;
			count = N;
		}

		auto count1 = std::min(count, N - write_idx);
		auto count2 = count - count1;
		auto it = buffer.begin();
		std::copy(data, data + count1, it + write_idx);
		std::copy(data + count1, data + count1 + count2, it);
		write_idx = (write_idx + count) % N;
		size = std::min(size + count, N);

		return ret;
	}

	size_t read_locked(char *data, size_t count)
	{
		std::lock_guard lock(mtx);
		return read(data, count);
	}

	void read_fill_zero_locked(char *data, size_t count)
	{
		std::lock_guard loc(mtx);
		read_fill_zero(data, count);
	}

	size_t write_locked(const char *data, size_t count)
	{
		std::lock_guard lock(mtx);
		return write(data, count);
	}

	void fill(char c) { buffer.fill(c); }

	void fill_locked(char c)
	{
		std::lock_guard lock(mtx);
		fill(c);
	}

      private:
	size_t size{};
	size_t read_idx{};
	size_t write_idx{};
	std::array<char, N> buffer{};
	std::mutex mtx;
};

} // namespace twice

#endif
