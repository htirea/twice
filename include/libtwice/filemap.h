#ifndef LIBTWICE_FILEMAP_H
#define LIBTWICE_FILEMAP_H

#include <cstddef>
#include <string>

namespace twice {

struct FileMap {
	static constexpr int MAP_EXACT_SIZE = 0x1;
	static constexpr int MAP_MAX_SIZE = 0x2;

	FileMap() = default;
	FileMap(const std::string& pathname, std::size_t limit, int mode);

	~FileMap() { destroy(); }

	FileMap(const FileMap& other) = delete;
	FileMap& operator=(const FileMap& other) = delete;

	FileMap(FileMap&& other)
	{
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0;
	}

	FileMap& operator=(FileMap&& other)
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

	explicit operator bool() { return data; }

	unsigned char *get_data() { return data; }

	size_t get_size() { return size; }

      private:
	unsigned char *data{};
	std::size_t size{};

	void destroy();
};

} // namespace twice

#endif
