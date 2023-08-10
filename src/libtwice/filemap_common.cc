#include "libtwice/filemap.h"

namespace twice {

file_map::~file_map()
{
	destroy();
}

file_map::file_map(file_map&& other) noexcept
{
	data = other.data;
	size = other.size;
	other.data = nullptr;
	other.size = 0;
}

file_map&
file_map::operator=(file_map&& other) noexcept
{
	if (this != &other) {
		destroy();

		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0;
	}

	return *this;
}

} // namespace twice
