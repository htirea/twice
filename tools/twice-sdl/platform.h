#ifndef TWICE_SDL_PLATFORM_H
#define TWICE_SDL_PLATFORM_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "SDL.h"

#include "libtwice/exception.h"
#include "libtwice/machine.h"

#include "moving_average.h"

namespace twice {

struct nds_machine;

struct sdl_platform_config {
	SDL_ScaleMode scale_mode = SDL_ScaleModeLinear;
	int window_scale = 2;
	bool fullscreen = false;
};

extern sdl_platform_config sdl_config;

class sdl_platform {
      public:
	struct sdl_error : twice_exception {
		using twice_exception::twice_exception;
	};

	sdl_platform();
	~sdl_platform();

	void loop(nds_machine *nds);

      private:
	void render(void *fb);
	void setup_default_binds();
	void handle_events(nds_machine *nds);
	void handle_key_event(nds_machine *nds, SDL_Keycode key, bool down);
	void handle_controller_button_event(
			nds_machine *nds, int button, bool down);
	void arm_set_title_fps(
			std::uint64_t ticks_per_frame, std::uint64_t freq);
	void add_controller(int joystick_index);
	void remove_controller(SDL_JoystickID id);
	void update_touchscreen_state(nds_machine *nds);
	void update_rtc(nds_machine *nds);
	void take_screenshot(void *fb);

	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};
	std::unordered_set<SDL_JoystickID> controllers;
	int window_w{};
	int window_h{};

	std::unordered_map<SDL_Keycode, nds_button> key_map;
	std::unordered_map<int, nds_button> button_map;

	bool running{};
	bool throttle{};

	moving_average<std::uint64_t> fps_counter;
};

std::string get_data_dir();

} // namespace twice

#endif
