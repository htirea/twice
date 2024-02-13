#include "file/posix_internal.h"

#include <cerrno>
#include <format>

#include <fcntl.h>
#include <sys/stat.h>

namespace twice::fs {

file::file() = default;

file::file(const std::filesystem::path& pathname, int flags)
{
	int err = open(pathname, flags);
	if (err) {
		throw file_error(std::format(
				"Could not open file: {}, error code: {}",
				pathname.string(), errno));
	}
}

file::file(file&&) = default;
file& file::operator=(file&&) = default;
file::~file() = default;

int
file::open(const std::filesystem::path& pathname, int flags)
{
	int oflags = 0;
	if (flags & open_flags::READ && flags & open_flags::WRITE) {
		oflags |= O_RDWR;
	} else if (flags & open_flags::READ) {
		oflags |= O_RDONLY;
	} else if (flags & open_flags::WRITE) {
		oflags |= O_WRONLY;
	} else {
		errno = -2;
		return -1;
	}

	mode_t mode = 0;
	if (flags & open_flags::CREATE) {
		oflags |= O_CREAT;
		if (flags & open_flags::NOREPLACE) {
			oflags |= O_EXCL;
		}
		mode = 0644;
	}

	int fd = ::open(pathname.c_str(), oflags, mode);
	if (fd < 0) {
		return -1;
	}

	internal = std::make_unique<impl>();
	internal->pathname = pathname;
	internal->fd = fd;

	return 0;
}

int
file::truncate(std::streamoff length)
{
	if (!*this) {
		errno = -1;
		return -1;
	}

	return ::ftruncate(internal->fd, length);
}

std::streamoff
file::read_from_offset(std::streamoff offset, void *buf, size_t count)
{
	if (!*this) {
		errno = -1;
		return -1;
	}

	off_t err = ::lseek(internal->fd, offset, SEEK_SET);
	if (err == -1) {
		return -1;
	}

	return ::read(internal->fd, buf, count);
}

int
file::sync()
{
	if (!*this) {
		errno = -1;
		return -1;
	}

	return ::fsync(internal->fd);
}

std::filesystem::path
file::get_pathname() const
{
	if (!*this)
		return {};

	return internal->pathname;
}

std::streamoff
file::get_size() const
{
	if (!*this) {
		errno = -1;
		return -1;
	}

	struct stat s;
	if (::fstat(internal->fd, &s)) {
		return -1;
	}

	return s.st_size;
}

file_view
file::pmap()
{
	return file_view(*this, file_view::map_flags::PRIVATE);
}

file_view
file::smap()
{
	return file_view(*this, file_view::map_flags::SHARED);
}

file::operator bool() const noexcept
{
	return internal && internal->fd != -1;
}

file::impl::~impl()
{
	if (fd != -1) {
		::close(fd);
	}
}

} // namespace twice::fs
