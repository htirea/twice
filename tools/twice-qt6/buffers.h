#ifndef TWICE_QT_BUFFERS_H
#define TWICE_QT_BUFFERS_H

#include "libtwice/nds/defs.h"
#include "libtwice/types.h"
#include "libtwice/util/triple_buffer.h"

struct SharedBuffers {
	using video_buffer = twice::triple_buffer<
			std::array<twice::u32, twice::NDS_FB_SZ>>;

	using audio_buffer =
			twice::triple_buffer<std::array<twice::s16, 2048>>;

	video_buffer vb{ {} };
	audio_buffer ab{ {} };
};

#endif
