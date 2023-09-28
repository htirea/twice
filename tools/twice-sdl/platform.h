#ifndef TWICE_SDL_PLATFORM_H
#define TWICE_SDL_PLATFORM_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "SDL.h"

#include "libtwice/exception.h"
#include "libtwice/nds/machine.h"

#include "moving_average.h"

namespace twice {

struct nds_machine;

enum {
	SCALE_MODE_NEAREST,
	SCALE_MODE_LINEAR,
	SCALE_MODE_HYBRID,
};

struct sdl_platform_config {
	int scale_mode{ SCALE_MODE_HYBRID };
	int window_scale{ 2 };
	bool fullscreen{ false };
};

extern sdl_platform_config sdl_config;

class sdl_platform {
      public:
	struct sdl_error : twice_exception {
		using twice_exception::twice_exception;
	};

	sdl_platform(nds_machine *nds);
	~sdl_platform();

	void loop();

      private:
	void render(void *fb);
	void queue_audio(void *audiobuffer, u32 size);
	void setup_default_binds();
	void handle_events();
	void handle_key_event(SDL_Keycode key, bool down);
	void handle_controller_button_event(int button, bool down);
	void arm_set_title(std::uint64_t ticks_per_frame, std::uint64_t freq,
			std::pair<double, double> cpu_usage);
	void add_controller(int joystick_index);
	void remove_controller(SDL_JoystickID id);
	void update_touchscreen_state();
	void update_rtc();
	void take_screenshot(void *fb);
	void event_window_size_changed(int w, int h);
	void reset_window_size(int scale);
	void adjust_window_size(int step);
	void toggle_fullscreen();

	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *texture{};
	SDL_Texture *scaled_texture{};
	SDL_AudioDeviceID audio_dev;
	SDL_AudioSpec audio_spec;
	std::unordered_set<SDL_JoystickID> controllers;
	std::unordered_map<SDL_Keycode, nds_button> key_map;
	std::unordered_map<int, nds_button> button_map;
	bool ctrl_down{};
	bool running{};
	bool throttle{};
	bool paused{};
	int window_w{};
	int window_h{};
	int texture_scale{};

	moving_average<std::uint64_t> fps_counter;
	nds_machine *nds{};
};

std::string get_data_dir();

} // namespace twice

#endif
