#ifndef LIBTWICE_FILE_H
#define LIBTWICE_FILE_H

#include "libtwice/exception.h"
#include "libtwice/types.h"

#include <filesystem>
#include <ios>
#include <memory>

namespace twice::fs {

struct file_error : twice_exception {
	using twice_exception::twice_exception;
};

struct file_view;

/**
 * A wrapper around a host file handle.
 *
 * The functions are heavily modelled around the POSIX syscalls.
 */
struct file {
	struct open_flags {
		enum {
			READ = 0x1,
			WRITE = 0x2,
			READ_WRITE = READ | WRITE,
			CREATE = 0x4,
			NOREPLACE = 0x8,
		};
	};

	/**
	 * Create a file handle with no underlying file.
	 */
	file();

	/**
	 * Create a file handle.
	 *
	 * \param pathname the path to the file
	 * \param flags the flags to use
	 */
	file(const std::filesystem::path& pathname, int flags);

	file(file&&);
	file& operator=(file&&);
	~file();

	/**
	 * Open a file.
	 *
	 * The `flags` parameter can be used to control the behaviour:
	 * `READ`: open for reading
	 * `WRITE`: open for writing
	 * `READ_WRITE` open for reading and writing
	 * `CREATE`: create file if `pathname` does not exist
	 * `NOREPLACE`: if `CREATE` is also set, ensure that this call created
	 *              the file
	 *
	 * \param pathname the path to the file
	 * \param flags the flags to use
	 */
	int open(const std::filesystem::path& pathname, int flags);

	/**
	 * Truncate the file.
	 *
	 * \param length the new length
	 * \returns 0 iff success
	 */
	int truncate(std::streamoff length);

	/**
	 * Duplicate the file handle.
	 *
	 * \returns the duplicated file handle.
	 */
	file dup();

	/**
	 * Seek to a specified offset from the beginning of the file.
	 *
	 * \returns the result offset on success
	 *          -1 on failure
	 */
	std::streamoff seek(std::streamoff offset);

	/**
	 * Read bytes.
	 *
	 * \param buf the destination buffer
	 * \param count the number of bytes
	 *
	 * \returns the number of bytes read, or -1 on error
	 */
	std::streamoff read(void *buf, size_t count);

	/**
	 * Read an exact amount of bytes.
	 *
	 * \param buf the destination buffer
	 * \param count the number of bytes
	 *
	 * \returns the number of bytes read, or -1 on error
	 */
	std::streamoff read_exact(void *buf, size_t count);

	/**
	 * Read an exact amount of bytes from an offset.
	 *
	 * \param offset the offset
	 * \param buf the destination buffer
	 * \param count the number of bytes
	 *
	 * \returns the number of bytes read, or -1 on error
	 */
	std::streamoff read_exact_offset(
			std::streamoff offset, void *buf, size_t count);

	/**
	 * Write bytes.
	 *
	 * \param buf the source buffer
	 * \param count the number of bytes
	 *
	 * \returns the number of bytes written, or -1 on error
	 */
	std::streamoff write(const void *buf, size_t count);

	/**
	 * Write an exact amount of bytes.
	 *
	 * \param buf the source buffer
	 * \param count the number of bytes
	 *
	 * \returns the number of bytes written, or -1 on error
	 */
	std::streamoff write_exact(const void *buf, size_t count);

	/**
	 * Write an exact amount of bytes to an offset.
	 *
	 * \param offset the offset
	 * \param buf the source buffer
	 * \param count the number of bytes
	 *
	 * \returns the number of bytes read, or -1 on error
	 */
	std::streamoff write_exact_offset(
			std::streamoff offset, const void *buf, size_t count);

	/**
	 * Sync file to disk.
	 */
	int sync();

	/**
	 * Get the pathname of the underlying file.
	 *
	 * \returns the pathname
	 */
	std::filesystem::path get_pathname() const;

	/**
	 * Get the size of the underlying file.
	 *
	 * This function is slow.
	 *
	 * \returns the size of the file
	 */
	std::streamoff get_size() const;

	/**
	 * Conversion to bool.
	 *
	 * \returns true iff there is an underlying file
	 */
	explicit operator bool() const noexcept;

	/**
	 * Convenience function for getting a private file view
	 *
	 * \returns a private file view
	 */
	file_view pmap();

	/**
	 * Convenience function for getting a shared file view
	 *
	 * \returns a shared file view
	 */
	file_view smap();

	/**
	 * Convenience function for getting a copy file view
	 *
	 * \returns a copy file view
	 */
	file_view cmap();

      private:
	struct impl;
	friend struct file_view;

	std::unique_ptr<impl> internal;
};

} // namespace twice::fs

#endif
