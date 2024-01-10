#ifndef LIBTWICE_THREADED_QUEUE
#define LIBTWICE_THREADED_QUEUE

#include <mutex>
#include <queue>

namespace twice {

template <typename T>
struct threaded_queue {
	int try_pop(T& dst)
	{
		std::lock_guard lock(mtx);
		if (q.empty()) {
			return 0;
		} else {
			dst = q.front();
			q.pop();
			return 1;
		}
	}

	void push(const T& v)
	{
		std::lock_guard lock(mtx);
		q.push(v);
	}

	void push(T&& v)
	{
		std::lock_guard lock(mtx);
		q.push(v);
	}

      private:
	std::queue<T> q;
	std::mutex mtx;
};

} // namespace twice

#endif
