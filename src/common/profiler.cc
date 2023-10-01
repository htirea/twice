#include "common/profiler.h"

namespace twice {

void
profiler::sample(size_t i)
{
	if (i == 0) {
		last_sample_time = now();
	} else {
		auto diff = now() - last_sample_time;
		intervals[i - 1] += diff;
		last_sample_time - now();
	}
}

void
profiler::report()
{
	tp::duration total(0);

	for (auto const& v : intervals) {
		total += v.second;
	}

	for (size_t i = 0; i < intervals.size(); i++) {
		LOG("%lu: %.2f %%\n", i, 100.0 * intervals[i] / total);
	}
}

} // namespace twice
