#ifndef TWICE_SDL_PLATFORM_H
#define TWICE_SDL_PLATFORM_H

#include <cstdint>
#include <string>
#include <unordered_set>

#include "SDL.h"

#include "libtwice/exception.h"

namespace twice {

struct nds_machine;

struct sdl_platform_config {
	std::string render_scale_quality = "1";
	int window_scale = 2;
	bool fullscreen = false;
};

extern sdl_platform_config sdl_config;

struct elapsed_ticks_counter {
	static constexpr unsigned BUF_SIZE = 64;

	std::uint64_t buffer[BUF_SIZE]{};
	std::uint64_t sum{};
	unsigned int index{};

	void add(std::uint64_t elapsed)
	{
		sum += elapsed;
		sum -= buffer[index];
		buffer[index] = elapsed;
		index = (index + 1) % BUF_SIZE;
	}

	std::uint64_t get_average() { return sum / BUF_SIZE; }
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
	void arm_set_title_fps(
			std::uint64_t ticks_per_frame, std::uint64_t freq);
	void add_controller(int joystick_index);
	void remove_controller(SDL_JoystickID id);

	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};
	std::unordered_set<SDL_JoystickID> controllers;

	bool running{};
	bool throttle{};

	elapsed_ticks_counter tick_counter;
};

std::string get_data_dir();

} // namespace twice

#endif
