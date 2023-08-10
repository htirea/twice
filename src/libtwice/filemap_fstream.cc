#include "libtwice/filemap.h"

#include "libtwice/exception.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>

namespace twice {

file_map::file_map(const std::string& pathname, std::size_t limit, int mode)
{
	std::ifstream f(pathname, std::ios::binary | std::ios::ate);
	if (!f.good()) {
		throw file_map_error("could not open file: " + pathname);
	}

	auto pos = f.tellg();
	if (pos < 0 || (std::uint64_t)pos >= SIZE_MAX) {
		throw file_map_error("file size out of range: " + pathname);
	}

	std::size_t actual_size = pos;

	if (mode == MAP_EXACT_SIZE && actual_size != limit) {
		throw file_map_error("file does not match size: " + pathname);
	} else if (mode == MAP_MAX_SIZE && actual_size > limit) {
		throw file_map_error("file exceeds size: " + pathname);
	}

	char *buffer = (char *)malloc(actual_size);
	if (!buffer) {
		throw file_map_error("memory alloc failed: " + pathname);
	}

	f.seekg(0, std::ios_base::beg);
	if (!f) {
		throw file_map_error("seek failed: " + pathname);
	}

	f.read(buffer, actual_size);
	if ((std::uint64_t)f.gcount() != actual_size) {
		throw file_map_error("read failed: " + pathname);
	}

	data = (unsigned char *)buffer;
	size = actual_size;
}

void
file_map::destroy() noexcept
{
	if (data) {
		free(data);
	}
}

} // namespace twice
