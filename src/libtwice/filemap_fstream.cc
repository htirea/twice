#include "libtwice/exception.h"
#include "libtwice/types.h"
#include "libtwice/util/filemap.h"

#include "common/logger.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

namespace twice {

struct file_map::impl {
	~impl();
	int sync();

	char *buffer{};
	size_t size{};
	std::string pathname;
	bool is_private{};
};

static std::vector<char> read_file_into_vector(const char *pathname);
static int try_read_file(file_map::impl *internal, const char *pathname,
		std::size_t limit, int flags, bool create);

file_map::file_map() = default;

file_map::file_map(const std::string& pathname, std::size_t limit, int flags)
	: internal(std::make_unique<impl>())
{
	int err = 0;

	if (flags & FILEMAP_SHARED) {
		err = try_read_file(internal.get(), pathname.c_str(), limit,
				flags, true);
	} else if (flags & FILEMAP_PRIVATE) {
		err = try_read_file(internal.get(), pathname.c_str(), limit,
				flags, false);
	} else {
		throw file_map_error("map mode not specified: " + pathname);
	}

	if (err) {
		throw file_map_error("could not create file map: " + pathname);
	}

	internal->pathname = pathname;
	internal->is_private = !(flags & FILEMAP_SHARED);
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

file_map::impl::~impl()
{
	if (buffer) {
		sync();
		delete[] buffer;
	}
}

int
file_map::impl::sync()
{
	if (is_private)
		return 0;

	if (!buffer)
		return 0;

	std::ofstream f(pathname, std::ios_base::binary);
	return f.write(buffer, size) ? 0 : 1;
}

static std::vector<char>
read_file_into_vector(const char *pathname)
{
	std::ifstream f(pathname, std::ios_base::binary);
	return { std::istreambuf_iterator(f), {} };
}

static int
try_read_file(file_map::impl *internal, const char *pathname,
		std::size_t limit, int flags, bool create)
{
	std::vector<char> buffer = read_file_into_vector(pathname);
	size_t filesize = buffer.size();

	if (filesize == 0) {
		if (create) {
			internal->buffer = new char[limit]();
			filesize = limit;
		} else {
			LOG("file does not exist\n");
			return 1;
		}
	} else if ((flags & file_map::FILEMAP_EXACT) && filesize != limit) {
		LOG("file does not match size\n");
		return 1;
	} else if ((flags & file_map::FILEMAP_LIMIT) && filesize > limit) {
		LOG("file exceeds size\n");
		return 1;
	} else {
		internal->buffer = new char[filesize];
		std::memcpy(internal->buffer, buffer.data(), filesize);
	}

	internal->size = filesize;
	return 0;
}

} // namespace twice
