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

class sdl_platform {
      public:
	struct sdl_error : twice_exception {
		using twice_exception::twice_exception;
	};

	struct config {
		SDL_ScaleMode scale_mode{ SDL_ScaleModeLinear };
		int window_scale{ 2 };
		bool fullscreen{ false };
		bool throttle{ true };
		u64 frame_limit{};
	};

	sdl_platform(nds_machine *nds, const config& cfg);
	~sdl_platform();

	void loop();

      private:
	void render(u32 *fb);
	void queue_audio(s16 *audiobuffer, u32 size, u64 ticks);
	void setup_default_binds();
	void handle_events();
	void handle_key_event(SDL_Keycode key, bool down);
	void handle_controller_button_event(int button, bool down);
	void arm_set_title(
			u64 ticks, const std::pair<double, double>& cpu_usage);
	void add_controller(int joystick_index);
	void remove_controller(SDL_JoystickID id);
	void update_touchscreen_state();
	void update_rtc();
	void take_screenshot(void *fb);
	void event_window_size_changed(int w, int h);
	void reset_window_size(int scale);
	void adjust_window_size(int step);
	void toggle_fullscreen();
	void rotate_screen(int dir);

	config sdl_config;
	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture *textures[2]{};
	SDL_AudioDeviceID audio_dev;
	SDL_AudioSpec audio_spec;
	u64 freq{};
	std::unordered_set<SDL_JoystickID> controllers;
	std::unordered_map<SDL_Keycode, nds_button> key_map;
	std::unordered_map<int, nds_button> button_map;
	bool ctrl_down{};
	bool running{};
	bool throttle{};
	bool paused{};
	bool audio_muted{};
	bool step_frame{};
	bool use_16_bit_audio{};
	u64 frames{};
	u64 frame_limit{};
	int window_w{};
	int window_h{};
	int texture_scale{};
	int orientation{};
	moving_average<std::uint64_t> fps_counter;
	nds_machine *nds{};
	nds_exec exec_out;
};

std::string get_data_dir();

} // namespace twice

#endif
