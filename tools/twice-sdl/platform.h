#ifndef TWICE_SDL_PLATFORM_H
#define TWICE_SDL_PLATFORM_H

#include <string>
#include <unordered_set>

#include <SDL.h>

#include <libtwice/exception.h>

namespace twice {

struct nds_machine;

struct frame_rate_counter {
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

struct sdl_platform {
	struct sdl_error : twice_exception {
		using twice_exception::twice_exception;
	};

	sdl_platform();
	~sdl_platform();

	void render(void *fb);
	void loop(nds_machine *nds);
	void handle_events(nds_machine *nds);
	void arm_set_title_fps(int fps);
	void add_controller(int joystick_index);
	void remove_controller(SDL_JoystickID id);

	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};
	std::unordered_set<SDL_JoystickID> controllers;

	bool running{};

	frame_rate_counter fps_counter;
};

std::string get_data_dir();

} // namespace twice

#endif
