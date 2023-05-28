#ifndef TWICE_SDL_PLATFORM_H
#define TWICE_SDL_PLATFORM_H

#include <string>

#include <SDL2/SDL.h>

#include <libtwice/exception.h>

namespace twice {

struct Machine;

struct FpsCounter {
	static constexpr unsigned BUF_SIZE = 64;

	int buffer[BUF_SIZE]{};
	int sum{};
	unsigned int index{};

	void add(int fps)
	{
		sum += fps;
		sum -= buffer[index];
		buffer[index] = fps;
		index = (index + 1) % BUF_SIZE;
	}

	int get_average_fps() { return sum / BUF_SIZE; }
};

struct Platform {
	class SDLError : TwiceException {
		using TwiceException::TwiceException;
	};

	Platform();
	~Platform();

	void render(void *fb);
	void loop(Machine *nds);
	void handle_events(Machine *nds);
	void set_title_fps(int fps);

	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};

	bool running{};

	FpsCounter fps_counter;
};

std::string get_data_dir();

} // namespace twice

#endif
