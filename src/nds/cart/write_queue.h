#ifndef TWICE_CART_WRITE_QUEUE_H
#define TWICE_CART_WRITE_QUEUE_H

#include "common/types.h"

namespace twice {

namespace fs {
struct file;
}

struct file_write_queue {
	bool write_in_progress{};
	std::optional<std::pair<u32, u32>> dirty_interval;
};

void file_queue_add(
		file_write_queue *q, u8 *data, u32 size, u32 start, u32 end);
void file_queue_flush(file_write_queue *q, fs::file& f, u8 *data);

} // namespace twice

#endif
