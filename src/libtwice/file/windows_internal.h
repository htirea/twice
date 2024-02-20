#ifndef TWICE_FILE_WINDOWS_INTERNAL_H
#define TWICE_FILE_WINDOWS_INTERNAL_H

#include "libtwice/file/file.h"
#include "libtwice/file/file_view.h"

#include <Windows.h>
#include <fileapi.h>

namespace twice::fs {

struct file::impl {
	~impl();
	std::filesystem::path pathname;
	HANDLE fh{ INVALID_HANDLE_VALUE };
};

struct file_view::impl {
	~impl();
	void *addr{};
	size_t size{};
};

} // namespace twice::fs

#endif
