#ifndef LIBTWICE_FILE_VIEW_H
#define LIBTWICE_FILE_VIEW_H

#include "libtwice/types.h"

#include <memory>

namespace twice::fs {

struct file;

/**
 * A view of a file.
 */
struct file_view {
	struct map_flags {
		enum {
			PRIVATE = 0x1,
			SHARED = 0x2,
			COPY = 0x4,
		};
	};

	/**
	 * Create an empty file view.
	 */
	file_view();

	/**
	 * Create a file view of a file.
	 *
	 * \param fh a file handle
	 */
	file_view(file& fh, int flags);

	file_view(file_view&&);
	file_view& operator=(file_view&&);
	~file_view();

	/**
	 * Create a file view of a file.
	 *
	 * The `flags` parameter can control the behaviour:
	 * `PRIVATE`: create a private mapping
	 * `SHARED`: create a shared mapping
	 * `COPY`: create a view over a copy of the file
	 *
	 * \param fh a file handle
	 */
	int map(file& fh, int flags);

	/**
	 * Get a pointer to the first byte in the view.
	 *
	 * \returns a pointer to the first byte
	 */
	unsigned char *data();
	const unsigned char *data() const;

	/**
	 * Get the size of the view.
	 *
	 * \returns the size of the view
	 */
	size_t size() const;

	/**
	 * Conversion to bool.
	 *
	 * \returns true iff there is an underlying file
	 */
	explicit operator bool() const noexcept;

	/**
	 * Sync view.
	 *
	 * \returns 0 iff success
	 */
	int sync();

      private:
	struct impl;
	std::unique_ptr<impl> internal;
};

} // namespace twice::fs

#endif
