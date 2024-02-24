#include "libtwice/file/windows_internal.h"

namespace twice::fs {

file_view::file_view() = default;

file_view::file_view(file& fh, int flags)
{
	int err = map(fh, flags);
	if (err) {
		throw file_error(std::format(
				"Could not create file view: {}, error code: {}",
				fh.get_pathname().string(), err));
	}
}

file_view::file_view(file_view&&) = default;
file_view& file_view::operator=(file_view&&) = default;
file_view::~file_view() = default;

int
file_view::map(file& f, int flags)
{
	if (!f) {
		internal.reset();
		return 0;
	}

	DWORD flprotect = 0;
	DWORD map_access = 0;
	HANDLE fh = f.internal->fh;

	if (flags & map_flags::PRIVATE) {
		flprotect = PAGE_READONLY;
		map_access = FILE_MAP_COPY;
	} else if (flags & map_flags::SHARED) {
		flprotect = PAGE_READWRITE;
		map_access = FILE_MAP_ALL_ACCESS;
	} else if (flags & map_flags::COPY) {
		flprotect = PAGE_READONLY;
		map_access = FILE_MAP_COPY;
		fh = INVALID_HANDLE_VALUE;
	} else {
		return -2;
	}

	auto map_size = f.get_size();
	if (map_size < 0) {
		return -1;
	}

	DWORD map_size_lo = map_size & 0xFFFFFFFF;
	DWORD map_size_hi = map_size >> 32;

	HANDLE map_handle = CreateFileMapping(
			fh, NULL, flprotect, map_size_hi, map_size_lo, NULL);
	if (!map_handle) {
		return -1;
	}

	LPVOID addr = MapViewOfFile(map_handle, map_access, 0, 0, 0);
	CloseHandle(map_handle);

	if (!addr) {
		return -1;
	}

	if (flags & map_flags::COPY) {
		if (f.read_exact_offset(0, addr, map_size) < 0) {
			UnmapViewOfFile(addr);
			return -1;
		}
	}

	internal = std::make_unique<impl>();
	internal->addr = addr;
	internal->size = map_size;

	return 0;
}

unsigned char *
file_view::data()
{
	if (!*this)
		return nullptr;

	return (unsigned char *)internal->addr;
}

const unsigned char *
file_view::data() const
{
	if (!*this)
		return nullptr;

	return (const unsigned char *)internal->addr;
}

size_t
file_view::size() const
{
	if (!*this)
		return 0;

	return internal->size;
}

file_view::operator bool() const noexcept
{
	return internal && internal->addr;
}

int
file_view::sync()
{
	if (!*this)
		return -1;

	return !FlushViewOfFile(internal->addr, 0);
}

file_view::impl::~impl()
{
	if (addr) {
		UnmapViewOfFile(addr);
	}
}

} // namespace twice::fs
