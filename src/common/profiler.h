#ifndef TWICE_PROFILER_H
#define TWICE_PROFILER_H

#include <chrono>

#include "common/logger.h"
#include "common/types.h"

namespace twice {

struct profiler {
	using tp = std::chrono::time_point<std::chrono::steady_clock>;

	void sample(size_t i);

	void report();

      private:
	tp last_sample_time;
	std::map<size_t, tp::duration> intervals;

	tp now() { return std::chrono::steady_clock::now(); }
};

} // namespace twice

#endif
