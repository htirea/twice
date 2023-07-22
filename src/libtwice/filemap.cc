#include "libtwice/filemap.h"
#include "libtwice/exception.h"

#include <cstdint>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace twice {

file_map::file_map(const std::string& pathname, std::size_t limit, int mode)
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

	if (s.st_size < 0 || (std::uint64_t)s.st_size >= SIZE_MAX) {
		close(fd);
		throw file_map_error("file size out of range: " + pathname);
	}

	std::size_t actual_size = s.st_size;

	if (mode == MAP_EXACT_SIZE && actual_size != limit) {
		close(fd);
		throw file_map_error("file does not match size: " + pathname);
	} else if (mode == MAP_MAX_SIZE && actual_size > limit) {
		close(fd);
		throw file_map_error("file exceeds size: " + pathname);
	}

	void *addr = mmap(NULL, actual_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		throw file_map_error("mmap failed: " + pathname);
	}

	close(fd);
	data = (unsigned char *)addr;
	size = actual_size;
}

void
file_map::destroy() noexcept
{
	if (data) {
		munmap(data, size);
	}
}

} // namespace twice
