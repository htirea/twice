#ifndef TWICE_STOPWATCH_H
#define TWICE_STOPWATCH_H

#include <chrono>

namespace twice {

class stopwatch {
      public:
	using clock = std::chrono::steady_clock;
	using rep = clock::rep;
	using period = clock::period;
	using duration = clock::duration;
	using time_point = clock::time_point;

	stopwatch() {}

	duration start()
	{
		auto new_start_time = now();
		auto time_since_last_start = new_start_time - start_time;
		start_time = new_start_time;
		return time_since_last_start;
	}

	time_point now() { return clock::now(); }

      private:
	time_point start_time{};
};

template <typename T, typename Rep, typename Period>
constexpr T
to_seconds(std::chrono::duration<Rep, Period> duration)
{
	return std::chrono::duration_cast<std::chrono::duration<T>>(duration)
	                .count();
}

} // namespace twice

#endif
