#ifndef LIBTWICE_FRAME_TIMER_H
#define LIBTWICE_FRAME_TIMER_H

#include <chrono>
#include <iostream>
#include <thread>

#include "libtwice/util/stopwatch.h"

namespace twice {

struct frame_timer {
	using clock = stopwatch::clock;
	using rep = clock::rep;
	using period = clock::period;
	using duration = clock::duration;
	using time_point = clock::time_point;

	frame_timer(const duration& target_duration)
		: target_duration(target_duration)
	{
	}

	void start_frame()
	{
		auto t = now();
		if (target_point <= t) {
			target_point = t + target_duration;
		}
	}

	void wait_until_target()
	{
		std::this_thread::sleep_until(target_point);

		while (now() < target_point)
			;

		target_point += target_duration;
	}

	void set_sleep_threshold(std::chrono::milliseconds v)
	{
		sleep_threshold = v;
	}

	time_point now() { return tmr.now(); }

      private:
	stopwatch tmr;
	duration target_duration;
	std::chrono::milliseconds sleep_threshold{ 0 };
	time_point start_point{};
	time_point target_point{};
};

} // namespace twice

#endif
