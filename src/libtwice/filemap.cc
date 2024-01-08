#include "libtwice/filemap.h"
#include "libtwice/exception.h"
#include "libtwice/types.h"

#include "common/logger.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace twice {

struct file_map::impl {
	~impl();
	int sync();

	char *buffer{};
	size_t size{};
	std::string pathname;
};

static int check_filesize(
		int fd, std::size_t limit, int flags, std::size_t *size_out);
static int create_shared_mapping(file_map::impl *internal,
		const char *pathname, std::size_t limit, int flags);
static int create_private_mapping(file_map::impl *internal,
		const char *pathname, std::size_t limit, int flags);

file_map::file_map() = default;

file_map::file_map(const std::string& pathname, std::size_t limit, int flags)
	: internal(std::make_unique<impl>())
{
	int err = 0;

	if (flags & FILEMAP_SHARED) {
		err = create_shared_mapping(internal.get(), pathname.c_str(),
				limit, flags);
	} else if (flags & FILEMAP_PRIVATE) {
		err = create_private_mapping(internal.get(), pathname.c_str(),
				limit, flags);
	} else {
		throw file_map_error("map mode not specified: " + pathname);
	}

	if (err) {
		throw file_map_error("could not create file map: " + pathname);
	}
}

file_map::file_map(file_map&&) = default;
file_map& file_map::operator=(file_map&&) = default;
file_map::~file_map() = default;

file_map::operator bool() const noexcept
{
	return internal && internal->buffer;
}

unsigned char *
file_map::data() noexcept
{
	return internal ? (unsigned char *)internal->buffer : nullptr;
}

const unsigned char *
file_map::data() const noexcept
{
	return internal ? (const unsigned char *)internal->buffer : nullptr;
}

size_t
file_map::size() const noexcept
{
	return internal ? internal->size : 0;
}

int
file_map::sync()
{
	return internal ? internal->sync() : 0;
}

std::string
file_map::get_pathname() const noexcept
{
	return internal ? internal->pathname : "";
}

file_map::impl::~impl()
{
	if (buffer) {
		sync();
		munmap(buffer, size);
	}
}

int
file_map::impl::sync()
{
	if (!buffer)
		return 0;

	return msync(buffer, size, MS_SYNC);
}

static int
check_filesize(int fd, std::size_t limit, int flags, std::size_t *size_out)
{
	struct stat s;
	if (fstat(fd, &s)) {
		LOG("could not stat file\n");
		return 1;
	}

	if ((flags & file_map::FILEMAP_EXACT) &&
			std::cmp_not_equal(s.st_size, limit)) {
		LOG("file does not match size\n");
		return 1;
	}

	if ((flags & file_map::FILEMAP_LIMIT) &&
			std::cmp_greater(s.st_size, limit)) {
		LOG("file exceeds size\n");
		return 1;
	}

	size_t filesize = s.st_size;
	if (std::cmp_equal(filesize, s.st_size)) {
		*size_out = filesize;
		return 0;
	} else {
		return 1;
	}
}

static int
create_private_mapping(file_map::impl *internal, const char *pathname,
		std::size_t limit, int flags)
{
	int fd = open(pathname, O_RDONLY);
	if (fd == -1) {
		LOG("could not open file\n");
		return 1;
	}

	std::size_t filesize;
	if (check_filesize(fd, limit, flags, &filesize)) {
		close(fd);
		return 1;
	}

	void *addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_PRIVATE,
			fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		LOG("mmap failed\n");
		return 1;
	}

	close(fd);
	internal->buffer = (char *)addr;
	internal->size = filesize;
	internal->pathname = pathname;
	return 0;
}

static int
create_shared_mapping(file_map::impl *internal, const char *pathname,
		std::size_t limit, int flags)
{
	bool created = false;
	int fd = open(pathname, O_RDWR | O_CREAT | O_EXCL, 0644);
	if (fd >= 0) {
		created = true;
		if (ftruncate(fd, limit)) {
			close(fd);
			unlink(pathname);
			LOG("ftruncate failed\n");
			return 1;
		}
	} else if (fd == -1 && errno == EEXIST) {
		fd = open(pathname, O_RDWR);
	}
	if (fd == -1) {
		close(fd);
		if (created)
			unlink(pathname);
		LOG("could not open file\n");
		return 1;
	}

	std::size_t filesize;
	if (created) {
		filesize = limit;
	} else {
		if (check_filesize(fd, limit, flags, &filesize)) {
			close(fd);
			return 1;
		}
	}

	void *addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		if (created)
			unlink(pathname);
		LOG("mmap failed\n");
		return 1;
	}

	close(fd);
	internal->buffer = (char *)addr;
	internal->size = filesize;
	internal->pathname = pathname;
	return 0;
}

} // namespace twice
