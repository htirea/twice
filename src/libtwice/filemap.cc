#include "libtwice/util/filemap.h"
#include "libtwice/types.h"

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

	int fd{ -1 };
	char *buffer{};
	size_t size{};
	std::string pathname;
	int flags{};
};

static int check_filesize(
		int fd, std::size_t limit, int flags, std::size_t *size_out);
static void create_shared_mapping(file_map::impl *internal,
		const char *pathname, std::size_t limit, int flags);
static void create_private_mapping(file_map::impl *internal,
		const char *pathname, std::size_t limit, int flags);

file_map::file_map() = default;

file_map::file_map(const std::filesystem::path& pathname, std::size_t limit,
		int flags)
	: internal(std::make_unique<impl>())
{
	int err = 0;

	if (flags & FILEMAP_SHARED) {
		create_shared_mapping(internal.get(), pathname.c_str(), limit,
				flags);
	} else if (flags & FILEMAP_PRIVATE) {
		create_private_mapping(internal.get(), pathname.c_str(), limit,
				flags);
	} else {
		throw file_map_error("File mapping mode not specified.");
	}

	if (err) {
		throw file_map_error("Could not create file map.");
	}
}

file_map::file_map(file_map&&) = default;
file_map& file_map::operator=(file_map&&) = default;
file_map::~file_map() = default;

void
file_map::remap()
{
	if (!(internal && internal->buffer))
		return;

	if (!(internal->flags & FILEMAP_PRIVATE))
		return;

	void *addr = mmap(NULL, internal->size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, internal->fd, 0);
	if (addr == MAP_FAILED) {
		throw file_map_error("Remap failed: mmap failed.");
	}

	if (munmap(internal->buffer, internal->size)) {
		throw file_map_error("Remap failed: munmap failed.");
	}

	internal->buffer = (char *)addr;
}

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

std::filesystem::path
file_map::get_pathname() const noexcept
{
	return internal ? internal->pathname : "";
}

file_map::impl::~impl()
{
	if (buffer) {
		sync();
		munmap(buffer, size);
		close(fd);
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
		return 1;
	}

	if ((flags & file_map::FILEMAP_EXACT) &&
			std::cmp_not_equal(s.st_size, limit)) {
		return 1;
	}

	if ((flags & file_map::FILEMAP_LIMIT) &&
			std::cmp_greater(s.st_size, limit)) {
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

static void
create_private_mapping(file_map::impl *internal, const char *pathname,
		std::size_t limit, int flags)
{
	int fd = open(pathname, O_RDONLY);
	if (fd == -1) {
		throw file_map::file_map_error("Could not open the file.");
	}

	std::size_t filesize;
	if (check_filesize(fd, limit, flags, &filesize)) {
		close(fd);
		throw file_map::file_map_error("Invalid file size.");
	}

	void *addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_PRIVATE,
			fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		throw file_map::file_map_error("mmap failed.");
	}

	internal->fd = fd;
	internal->buffer = (char *)addr;
	internal->size = filesize;
	internal->pathname = pathname;
	internal->flags = flags;
}

static void
create_shared_mapping(file_map::impl *internal, const char *pathname,
		std::size_t limit, int flags)
{
	bool created = false;

	int fd;
	if (flags & file_map::FILEMAP_MUST_EXIST) {
		fd = open(pathname, O_RDWR);
	} else {
		fd = open(pathname, O_RDWR | O_CREAT | O_EXCL, 0644);
		if (fd >= 0) {
			created = true;
			if (ftruncate(fd, limit)) {
				close(fd);
				unlink(pathname);
				throw file_map::file_map_error(
						"ftruncate failed.");
			}
		} else if (fd == -1 && errno == EEXIST) {
			fd = open(pathname, O_RDWR);
		}
	}

	if (fd == -1) {
		close(fd);
		if (created)
			unlink(pathname);
		throw file_map::file_map_error("Could not open the file.");
	}

	std::size_t filesize;
	if (created) {
		filesize = limit;
	} else {
		if (check_filesize(fd, limit, flags, &filesize)) {
			close(fd);
			throw file_map::file_map_error("Invalid file size.");
		}
	}

	void *addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		if (created)
			unlink(pathname);
		throw file_map::file_map_error("mmap failed.");
	}

	internal->fd = fd;
	internal->buffer = (char *)addr;
	internal->size = filesize;
	internal->pathname = pathname;
	internal->flags = flags;
}

} // namespace twice
