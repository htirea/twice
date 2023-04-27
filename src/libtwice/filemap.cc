#include "libtwice/filemap.h"
#include "libtwice/exception.h"
#include "libtwice/types.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace twice;

FileMap::FileMap(const std::string& pathname, std::size_t limit, int mode)
{
	int fd = open(pathname.c_str(), O_RDONLY);
	if (fd == -1) {
		throw TwiceFileError("could not open file: " + pathname);
	}

	struct stat s;
	if (fstat(fd, &s)) {
		close(fd);
		throw TwiceFileError("could not stat file: " + pathname);
	}

	size_t actual_size = s.st_size;

	if (mode == MAP_EXACT_SIZE && actual_size != limit) {
		close(fd);
		throw TwiceFileError("file does not match size: " + pathname);
	} else if (mode == MAP_MAX_SIZE && actual_size > limit) {
		close(fd);
		throw TwiceFileError("file exceeds size: " + pathname);
	}

	auto addr = mmap(NULL, actual_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		throw TwiceFileError("mmap failed: " + pathname);
	}

	this->data = (unsigned char *)addr;
	this->size = actual_size;
}

void
FileMap::destroy()
{
	if (data) {
		munmap(data, size);
	}
}
