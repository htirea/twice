#ifndef TWICE_FRAME_TIMER_H
#define TWICE_FRAME_TIMER_H

#include <chrono>
#include <iostream>
#include <thread>

namespace twice {

class frame_timer {
      public:
	using clock = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<clock>;
	using duration = time_point::duration;

	frame_timer(const duration& interval_target)
		: interval_target(interval_target)
	{
	}

	void start_interval()
	{
		auto t = now();
		last_period = t - interval_start_time;
		interval_start_time = t;
	}

	void end_interval()
	{
		time_point t = now();
		elapsed = t - interval_start_time;

		if (elapsed >= interval_target) {
			time_until_target = duration::zero();
			accumulated_delay += elapsed - interval_target;
		} else {
			time_until_target = interval_target - elapsed;
			auto dt = std::min(
					time_until_target, accumulated_delay);
			accumulated_delay -= dt;
			time_until_target -= dt;
		}
	}

	void throttle()
	{
		auto target = interval_start_time + elapsed +
		              time_until_target;
		auto min_sleep = std::chrono::milliseconds(4);
		if (time_until_target > min_sleep) {
			std::this_thread::sleep_for(
					time_until_target - min_sleep);
		}

		while (now() < target)
			;
	}

	duration get_last_period() { return last_period; }

	time_point now() { return clock::now(); }

      private:
	time_point interval_start_time{};
	duration last_period{};
	duration elapsed{};
	duration interval_target{};
	duration accumulated_delay{};
	duration time_until_target{};
};

} // namespace twice

#endif
