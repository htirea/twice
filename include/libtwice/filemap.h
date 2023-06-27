#ifndef TWICE_FILEMAP_H
#define TWICE_FILEMAP_H

#include "libtwice/exception.h"

#include <cstddef>
#include <string>

namespace twice {

struct file_map {
	struct file_map_error : twice_exception {
		using twice_exception::twice_exception;
	};

	static constexpr int MAP_EXACT_SIZE = 0x1;
	static constexpr int MAP_MAX_SIZE = 0x2;

	file_map() noexcept = default;
	file_map(const std::string& pathname, std::size_t limit, int mode);

	~file_map() { destroy(); }

	file_map(const file_map& other) = delete;
	file_map& operator=(const file_map& other) = delete;

	file_map(file_map&& other) noexcept
	{
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0;
	}

	file_map& operator=(file_map&& other) noexcept
	{
		if (this != &other) {
			destroy();

			data = other.data;
			size = other.size;
			other.data = nullptr;
			other.size = 0;
		}

		return *this;
	}

	explicit operator bool() noexcept { return data; }

	unsigned char *get_data() noexcept { return data; }

	size_t get_size() noexcept { return size; }

      private:
	unsigned char *data{};
	std::size_t size{};

	void destroy() noexcept;
};

} // namespace twice

#endif
