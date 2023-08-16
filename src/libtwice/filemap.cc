#include "libtwice/filemap.h"
#include "libtwice/exception.h"
#include "libtwice/types.h"

#include <cstdint>
#include <limits>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace twice {

template <typename From, typename To>
std::pair<bool, To>
safe_convert(From x)
{

	constexpr bool fs = std::numeric_limits<From>::is_signed;
	constexpr bool ts = std::numeric_limits<To>::is_signed;
	constexpr To low = std::numeric_limits<To>::min();
	constexpr To high = std::numeric_limits<To>::max();
	bool safe;

	if constexpr (fs && ts) { /* signed to signed */
		safe = low <= (intmax_t)x && (intmax_t)x <= high;
	} else if constexpr (fs) { /* signed to unsigned */
		safe = x >= 0 && (uintmax_t)x <= high;
	} else if constexpr (ts) { /* unsigned to signed */
		safe = (uintmax_t)x <= (uintmax_t)high;
	} else { /* unsigned to unsigned */
		safe = (uintmax_t)x <= high;
	}

	return { safe, static_cast<To>(x) };
}

file_map::file_map(const std::string& pathname, std::size_t limit, int flags)
{
	if (flags & FILEMAP_SHARED) {
		create_shared_mapping(pathname, limit);
	} else if (flags & FILEMAP_PRIVATE) {
		create_private_mapping(pathname, limit, flags);
	} else {
		throw file_map_error("map mode not specified: " + pathname);
	}
}

void
file_map::create_private_mapping(
		const std::string& pathname, std::size_t limit, int flags)
{
	int fd = open(pathname.c_str(), O_RDONLY);
	if (fd == -1) {
		throw file_map_error("could not open file: " + pathname);
	}

	struct stat s;
	if (fstat(fd, &s)) {
		close(fd);
		throw file_map_error("could not stat file: " + pathname);
	}

	auto [safe, filesize] = safe_convert<off_t, size_t>(s.st_size);
	if (!safe) {
		close(fd);
		throw file_map_error("file size out of range: " + pathname);
	}

	if ((flags & FILEMAP_EXACT) && filesize != limit) {
		close(fd);
		throw file_map_error("file does not match size: " + pathname);
	} else if ((flags & FILEMAP_LIMIT) && filesize > limit) {
		close(fd);
		throw file_map_error("file exceeds size: " + pathname);
	}

	void *addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_PRIVATE,
			fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		throw file_map_error("mmap failed: " + pathname);
	}

	close(fd);
	data_p = (unsigned char *)addr;
	mapped_size = filesize;
}

void
file_map::create_shared_mapping(const std::string& pathname, std::size_t limit)
{
	bool created = false;
	int fd = open(pathname.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
	if (fd >= 0) {
		created = true;
	} else if (fd == -1 && errno == EEXIST) {
		fd = open(pathname.c_str(), O_RDWR);
	}
	if (fd == -1) {
		close(fd);
		if (created) unlink(pathname.c_str());
		throw file_map_error("could not open file: " + pathname);
	}

	struct stat s;
	if (fstat(fd, &s)) {
		close(fd);
		if (created) unlink(pathname.c_str());
		throw file_map_error("could not stat file: " + pathname);
	}

	auto [safe, filesize] = safe_convert<off_t, size_t>(s.st_size);
	if (!safe) {
		close(fd);
		if (created) unlink(pathname.c_str());
		throw file_map_error("filesize out of range: " + pathname);
	}

	if (filesize < limit) {
		auto [safe, new_size] = safe_convert<size_t, off_t>(limit);
		if (!safe || ftruncate(fd, new_size)) {
			close(fd);
			if (created) unlink(pathname.c_str());
			throw file_map_error("ftruncate failed: " + pathname);
		}
		filesize = limit;
	}

	void *addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		if (created) unlink(pathname.c_str());
		throw file_map_error("mmap failed: " + pathname);
	}

	close(fd);
	data_p = (unsigned char *)addr;
	mapped_size = filesize;
}

void
file_map::destroy() noexcept
{
	if (data_p) {
		munmap(data_p, mapped_size);
	}
}

} // namespace twice
