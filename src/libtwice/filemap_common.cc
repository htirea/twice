#include "libtwice/filemap.h"

namespace twice {

file_map::~file_map()
{
	destroy();
}

file_map::file_map(file_map&& other) noexcept
{
	data_p = other.data_p;
	mapped_size = other.mapped_size;
	other.data_p = nullptr;
	other.mapped_size = 0;
}

file_map&
file_map::operator=(file_map&& other) noexcept
{
	if (this != &other) {
		destroy();

		data_p = other.data_p;
		mapped_size = other.mapped_size;
		other.data_p = nullptr;
		other.mapped_size = 0;
	}

	return *this;
}

} // namespace twice
