#include "nds/cart/write_queue.h"

#include "common/logger.h"
#include "libtwice/file/file.h"

namespace twice {

void
file_queue_add(file_write_queue *q, u8 *data, u32 size, u32 start, u32 end)
{
	if (start == end || start >= size)
		return;

	if (end > size) {
		end = size;
	}

	auto [dirty_start, dirty_end] =
			q->dirty_interval.value_or(std::make_pair(start, end));
	dirty_start = std::min(dirty_start, start);
	dirty_end = std::max(dirty_end, end);
	q->dirty_interval = { dirty_start, dirty_end };
}

void
file_queue_flush(file_write_queue *q, fs::file& f, u8 *data)
{
	if (q->write_in_progress)
		return;

	if (!q->dirty_interval)
		return;

	auto [start, end] = q->dirty_interval.value();
	q->dirty_interval.reset();

	auto count = end - start;
	auto bytes_written = f.write_exact_offset(start, data + start, count);
	if (bytes_written != (std::streamoff)count) {
		LOG("error while writing to file\n");
	} else {
		LOGV("wrote to file: offset %u, count %ld\n", start,
				bytes_written);
	}
}
} // namespace twice
