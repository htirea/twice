#include "libtwice/file/file.h"
#include "libtwice/file/file_view.h"

namespace twice::fs {

std::streamoff
file::read_exact(void *buf, size_t count)
{
	size_t bytes_left = count;

	do {
		auto bytes_read = read(buf, bytes_left);
		if (bytes_read < 0) {
			return -1;
		}
		bytes_left -= bytes_read;
	} while (bytes_left > 0);

	return count;
}

std::streamoff
file::read_exact_offset(std::streamoff offset, void *buf, size_t count)
{
	auto seek_result = seek(offset);
	if (seek_result < 0) {
		return seek_result;
	}

	return read_exact(buf, count);
}

std::streamoff
file::write_exact(const void *buf, size_t count)
{
	size_t bytes_left = count;

	do {
		auto bytes_written = write(buf, bytes_left);
		if (bytes_written < 0) {
			return -1;
		}
		bytes_left -= bytes_written;
	} while (bytes_left > 0);

	return count;
}

std::streamoff
file::write_exact_offset(std::streamoff offset, const void *buf, size_t count)
{
	auto seek_result = seek(offset);
	if (seek_result < 0) {
		return seek_result;
	}

	return write_exact(buf, count);
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

file_view
file::cmap()
{
	return file_view(*this, file_view::map_flags::COPY);
}

} // namespace twice::fs
