#ifndef TWICE_FILE_POSIX_INTERNAL_H
#define TWICE_FILE_POSIX_INTERNAL_H

#include "libtwice/file/file.h"
#include "libtwice/file/file_view.h"

namespace twice::fs {

struct file::impl {
	~impl();
	std::filesystem::path pathname;
	int fd{ -1 };
};

struct file_view::impl {
	~impl();
	void *addr{};
	size_t size{};
};

} // namespace twice::fs

#endif
