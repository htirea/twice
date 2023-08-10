#ifndef LIBTWICE_FILEMAP_H
#define LIBTWICE_FILEMAP_H

#include "libtwice/exception.h"

#include <cstddef>
#include <string>

namespace twice {

/**
 * Provides access to a file through a pointer.
 */
struct file_map {
	struct file_map_error : twice_exception {
		using twice_exception::twice_exception;
	};

	enum map_flags {
		MAP_EXACT_SIZE = 0x1,
		MAP_MAX_SIZE = 0x2,
	};

	/**
	 * Create a file mapping with no underlying file.
	 */
	file_map() noexcept = default;

	/**
	 * Create a file mapping.
	 *
	 * The `limit` parameter can be used to limit the size of the mapping.
	 *
	 * The `flags` parameter can be used control the behaviour of the file
	 * mapping. The following flags can be used:
	 *
	 * `MAP_EXACT_SIZE`: the size of the file must match the limit
	 * `MAP_MAX_SIZE`: the size of the must not exceed the limit
	 *
	 * \param pathname the path to the file
	 * \param limit the maximum size of the mapping
	 * \param flags the flags to use
	 */
	file_map(const std::string& pathname, std::size_t limit, int flags);
	~file_map();
	file_map(const file_map& other) = delete;
	file_map& operator=(const file_map& other) = delete;
	file_map(file_map&& other) noexcept;
	file_map& operator=(file_map&& other) noexcept;

	/**
	 * Conversion to bool.
	 *
	 * \returns true if there is an underlying file
	 *          false otherwise
	 */
	explicit operator bool() noexcept { return data; }

	/**
	 * Get a pointer to the underlying file.
	 *
	 * \returns a pointer to the underlying file
	 */
	unsigned char *get_data() noexcept { return data; }

	/**
	 * Get the size of the underlying file.
	 *
	 * \returns the size of the underlying file
	 */
	size_t get_size() noexcept { return size; }

      private:
	unsigned char *data{};
	std::size_t size{};

	void destroy() noexcept;
};

} // namespace twice

#endif
