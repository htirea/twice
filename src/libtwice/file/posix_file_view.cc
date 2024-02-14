#include "libtwice/file/posix_internal.h"

#include <cerrno>
#include <format>

#include <sys/mman.h>

namespace twice::fs {

file_view::file_view() = default;

file_view::file_view(const file& fh, int flags)
{
	int err = map(fh, flags);
	if (err) {
		throw file_error(std::format(
				"Could not create file view: {}, error code: {}",
				fh.get_pathname().string(), errno));
	}
}

file_view::file_view(file_view&&) = default;
file_view& file_view::operator=(file_view&&) = default;
file_view::~file_view() = default;

int
file_view::map(const file& fh, int flags)
{
	if (!fh) {
		internal.reset();
		return 0;
	}

	int mflags = 0;
	if (flags & map_flags::PRIVATE) {
		mflags |= MAP_PRIVATE;
	} else if (flags & map_flags::SHARED) {
		mflags |= MAP_SHARED;
	} else {
		errno = -2;
		return -1;
	}

	std::streamoff map_size = fh.get_size();
	if (map_size < 0) {
		return -1;
	}

	void *addr = ::mmap(NULL, map_size, PROT_READ | PROT_WRITE, mflags,
			fh.internal->fd, 0);
	if (addr == MAP_FAILED) {
		return -1;
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

	return ::msync(internal->addr, internal->size, MS_SYNC);
}

file_view::impl::~impl()
{
	if (addr) {
		::munmap(addr, size);
	}
}

} // namespace twice::fs
