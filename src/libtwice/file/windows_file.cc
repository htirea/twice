#include "libtwice/file/windows_internal.h"

namespace twice::fs {

file::file() = default;

file::file(const std::filesystem::path& pathname, int flags)
{
	int err = open(pathname, flags);
	if (err) {
		throw file_error(std::format(
				"Could not open file: {}, error code: {}",
				pathname.string(), err));
	}
}

file::file(file&&) = default;
file& file::operator=(file&&) = default;
file::~file() = default;

int
file::open(const std::filesystem::path& pathname, int flags)
{
	DWORD desired_access = 0;
	if (flags & open_flags::READ || flags & open_flags::WRITE) {
		if (flags & open_flags::READ) {
			desired_access |= GENERIC_READ;
		}
		if (flags & open_flags::WRITE) {
			desired_access |= GENERIC_WRITE;
		}
	} else {
		return -2;
	}

	DWORD creation_disposition = 0;
	if (flags & open_flags::CREATE && flags & open_flags::NOREPLACE) {
		creation_disposition = CREATE_NEW;
	} else if (flags & open_flags::CREATE) {
		creation_disposition = OPEN_ALWAYS;
	} else {
		creation_disposition = OPEN_EXISTING;
	}

	auto fh = CreateFileW(pathname.wstring().data(), desired_access, 0,
			NULL, creation_disposition, FILE_ATTRIBUTE_NORMAL,
			NULL);
	if (fh == INVALID_HANDLE_VALUE) {
		return -1;
	}

	internal = std::make_unique<impl>();
	internal->pathname = pathname;
	internal->fh = fh;

	return 0;
}

int
file::truncate(std::streamoff length)
{
	if (!*this) {
		return -1;
	}

	LARGE_INTEGER distance;
	distance.QuadPart = length;

	if (!SetFilePointerEx(internal->fh, distance, NULL, FILE_BEGIN)) {
		return -1;
	}

	return !SetEndOfFile(internal->fh);
}

file
file::dup()
{
	if (!*this) {
		return file();
	}

	HANDLE fh;

	bool success = DuplicateHandle(GetCurrentProcess(), internal->fh,
			GetCurrentProcess(), &fh, 0, TRUE,
			DUPLICATE_SAME_ACCESS);
	if (!success) {
		throw file_error(std::format(
				"Could not copy file handle: {}, error: {}",
				internal->pathname.string(), -1));
	}

	file f;
	f.internal = std::make_unique<impl>();
	f.internal->pathname = internal->pathname;
	f.internal->fh = fh;
	return f;
}

std::streamoff
file::seek(std::streamoff offset)
{
	if (!*this) {
		return -1;
	}

	LARGE_INTEGER file_offset;
	file_offset.QuadPart = offset;
	return !SetFilePointerEx(internal->fh, file_offset, NULL, FILE_BEGIN);
}

std::streamoff
file::read(void *buf, size_t count)
{
	if (!*this) {
		return -1;
	}

	DWORD bytes_to_read = std::min<size_t>(count, 0x7FFFFFFF);
	DWORD bytes_read = 0;
	if (!ReadFile(internal->fh, buf, bytes_to_read, &bytes_read, NULL)) {
		return -1;
	}

	return bytes_read;
}

std::streamoff
file::write(const void *buf, size_t count)
{
	if (!*this) {
		return -1;
	}

	DWORD bytes_to_write = std::min<size_t>(count, 0x7FFFFFFF);
	DWORD bytes_written = 0;
	if (!WriteFile(internal->fh, buf, bytes_to_write, &bytes_written,
			    NULL)) {
		return -1;
	}

	return bytes_written;
}

int
file::sync()
{
	if (!*this) {
		return -1;
	}

	return !FlushFileBuffers(internal->fh);
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
		return -1;
	}

	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(internal->fh, &file_size)) {
		return -1;
	}

	return file_size.QuadPart;
}

file::operator bool() const noexcept
{
	return internal && internal->fh != INVALID_HANDLE_VALUE;
}

file::impl::~impl()
{
	if (fh != INVALID_HANDLE_VALUE) {
		CloseHandle(fh);
	}
}

} // namespace twice::fs
