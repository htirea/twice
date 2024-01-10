#ifndef LIBTWICE_FILEMAP_H
#define LIBTWICE_FILEMAP_H

#include "libtwice/exception.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace twice {

/**
 * Provides access to a file through a pointer.
 */
struct file_map {
	struct impl;

	struct file_map_error : twice_exception {
		using twice_exception::twice_exception;
	};

	enum map_flags {
		FILEMAP_PRIVATE = 0x1,
		FILEMAP_SHARED = 0x2,
		FILEMAP_EXACT = 0x4,
		FILEMAP_LIMIT = 0x8,
		FILEMAP_MUST_EXIST = 0x10,
	};

	/**
	 * Create a file mapping with no underlying file.
	 */
	file_map();

	/**
	 * Create a file mapping.
	 *
	 * The `limit` parameter can be used to limit the size of the mapping.
	 *
	 * The `flags` parameter can be used control the behaviour of the file
	 * mapping. The following flags can be used:
	 *
	 * `FILEMAP_PRIVATE`: changes to the mapped region are not carried
	 *                    through to the underlying file
	 * `FILEMAP_SHARED`: changes to the mapped region are carried through
	 *                   to the underlying file
	 * `FILEMAP_EXACT`: the size of the file must match the limit
	 * `FILEMAP_LIMIT`: the size of the file must not exceed the limit
	 * `FILEMAP_MUST_EXIST`: the file must exist
	 * `FILEMAP_MUST_NOT_EXIST`: the file must not exist
	 *
	 * If `FILEMAP_PRIVATE` is specified, the underlying file will be
	 * opened in read-only mode. The file must exist.
	 *
	 * If `FILEMAP_SHARED` is specified, the underlying file will be opened
	 * in read/write mode. .
	 *
	 * The mapped region is read/write-able regardless of permissions on
	 * the underlying file.
	 *
	 * \param pathname the path to the file
	 * \param limit the maximum size of the mapping
	 * \param flags the flags to use
	 */
	file_map(const std::filesystem::path& pathname, std::size_t limit,
			int flags);

	file_map(file_map&&);
	file_map& operator=(file_map&&);
	~file_map();

	/**
	 * Remap the file mapping.
	 *
	 * If the file was mapped with `FILEMAP_PRIVATE`, this function unmaps
	 * and remaps the same file. This can be used to discard any changes
	 * made to the mapping.
	 *
	 * If the file was mapped with `FILEMAP_SHARED`, this function does
	 * nothing.
	 */
	void remap();

	/**
	 * Conversion to bool.
	 *
	 * \returns true if there is an underlying file
	 *          false otherwise
	 */
	explicit operator bool() const noexcept;

	/**
	 * Get a pointer to the underlying file.
	 *
	 * \returns a pointer to the underlying file
	 */
	unsigned char *data() noexcept;

	const unsigned char *data() const noexcept;

	/**
	 * Get the size of the underlying file.
	 *
	 * \returns the size of the underlying file
	 */
	size_t size() const noexcept;

	/**
	 * Sync changes to the underlying file.
	 *
	 * If the file was mapped in private mode then this function does
	 * nothing.
	 *
	 * \returns 0 on success
	 */
	int sync();

	/**
	 * Get the pathname of the underlying file.
	 *
	 * \returns the pathname
	 */
	std::filesystem::path get_pathname() const noexcept;

      private:
	std::unique_ptr<impl> internal;
};

} // namespace twice

#endif
