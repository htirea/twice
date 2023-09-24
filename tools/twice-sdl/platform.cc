#include "platform.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>

#include "screenshot.h"

namespace twice {

static SDL_Texture *
create_scaled_texture(SDL_Renderer *renderer, int scale)
{
	SDL_Texture *texture = SDL_CreateTexture(renderer,
			SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_TARGET,
			NDS_FB_W * scale, NDS_FB_H * scale);
	SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
	return texture;
}

sdl_platform::sdl_platform(nds_machine *nds) : nds(nds)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)) {
		throw sdl_error("init failed");
	}

	Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (sdl_config.fullscreen) {
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	window_w = NDS_FB_W * sdl_config.window_scale;
	window_h = NDS_FB_H * sdl_config.window_scale;
	window = SDL_CreateWindow("Twice", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, window_w, window_h,
			window_flags);
	if (!window) {
		throw sdl_error("create window failed");
	}
	SDL_SetWindowResizable(window, SDL_TRUE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		throw sdl_error("create renderer failed");
	}
	if (SDL_RenderSetLogicalSize(renderer, NDS_FB_W, NDS_FB_H)) {
		throw sdl_error("renderer set logical size failed");
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
			SDL_TEXTUREACCESS_STREAMING, NDS_FB_W, NDS_FB_H);
	if (!texture) {
		throw sdl_error("create texture failed");
	}

	SDL_ScaleMode sdl_scale_mode;
	switch (sdl_config.scale_mode) {
	case SCALE_MODE_NEAREST:
	case SCALE_MODE_HYBRID:
		sdl_scale_mode = SDL_ScaleModeNearest;
		break;
	case SCALE_MODE_LINEAR:
		sdl_scale_mode = SDL_ScaleModeLinear;
		break;
	default:
		throw sdl_error("unknown scale mode");
	}
	if (SDL_SetTextureScaleMode(texture, sdl_scale_mode)) {
		throw sdl_error("set texture scale mode failed");
	}

	if (sdl_config.scale_mode == SCALE_MODE_HYBRID) {
		texture_scale = sdl_config.window_scale;
		scaled_texture =
				create_scaled_texture(renderer, texture_scale);
		if (!scaled_texture) {
			throw sdl_error("create scaled texture failed");
		}
	}

	int num_joysticks = SDL_NumJoysticks();
	for (int i = 0; i < num_joysticks; i++) {
		if (SDL_IsGameController(i)) {
			add_controller(i);
		}
	}

	setup_default_binds();
}

sdl_platform::~sdl_platform()
{
	while (!controllers.empty()) {
		SDL_JoystickID id = *controllers.begin();
		remove_controller(id);
	}

	SDL_DestroyTexture(scaled_texture);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

std::string
get_data_dir()
{
	std::string data_dir;

	char *path = SDL_GetPrefPath("", "twice");
	if (path) {
		data_dir = path;
		SDL_free(path);
	}

	return data_dir;
}

void
sdl_platform::render(void *fb)
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);

	int pitch;
	void *p;
	SDL_LockTexture(texture, NULL, &p, &pitch);
	std::memcpy(p, fb, NDS_FB_SZ_BYTES);
	SDL_UnlockTexture(texture);

	if (scaled_texture) {
		SDL_SetRenderTarget(renderer, scaled_texture);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, scaled_texture, NULL, NULL);
	} else {
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
	}

	SDL_RenderPresent(renderer);
}

void
sdl_platform::arm_set_title(std::uint64_t ticks_per_frame, std::uint64_t freq,
		std::pair<double, double> cpu_usage)
{
	double fps = (double)freq / ticks_per_frame;
	double frametime = 1000.0 * ticks_per_frame / freq;
	std::string title = std::format(
			"Twice [{:.2f} fps | {:.2f} ms | {:.2f} | {:.2f}]",
			fps, frametime, cpu_usage.first, cpu_usage.second);
	SDL_SetWindowTitle(window, title.c_str());
}

void
sdl_platform::loop()
{
	std::uint64_t freq = SDL_GetPerformanceFrequency();
	std::uint64_t tframe = freq / 59.8261;
	std::uint64_t start = SDL_GetPerformanceCounter();
	std::uint64_t ticks_elapsed = 0;

	running = true;
	throttle = true;

	while (running) {
		handle_events();
		nds->run_frame();
		render(nds->get_framebuffer());
		if (nds->is_shutdown()) {
			break;
		}

		if (throttle) {
			std::uint64_t elapsed =
					SDL_GetPerformanceCounter() - start;
			if (elapsed < tframe) {
				double remaining_ns = (tframe - elapsed) *
				                      1e9 / freq;
				if (remaining_ns > 4e6) {
					SDL_Delay((remaining_ns - 4e6) / 1e6);
				}
			}
			while (SDL_GetPerformanceCounter() - start < tframe)
				;
		}

		std::uint64_t elapsed = SDL_GetPerformanceCounter() - start;
		start = SDL_GetPerformanceCounter();

		fps_counter.insert(elapsed);

		ticks_elapsed += elapsed;
		if (ticks_elapsed >= freq) {
			ticks_elapsed -= freq;
			arm_set_title(fps_counter.get_average(), freq,
					nds->get_cpu_usage());
			update_rtc();
		}
	}

	running = false;
}

void
sdl_platform::setup_default_binds()
{
	using enum nds_button;

	key_map = {
		{ SDLK_x, A },
		{ SDLK_z, B },
		{ SDLK_s, X },
		{ SDLK_a, Y },
		{ SDLK_w, R },
		{ SDLK_q, L },
		{ SDLK_1, START },
		{ SDLK_2, SELECT },
		{ SDLK_LEFT, LEFT },
		{ SDLK_RIGHT, RIGHT },
		{ SDLK_UP, UP },
		{ SDLK_DOWN, DOWN },
	};

	button_map = {
		{ SDL_CONTROLLER_BUTTON_B, A },
		{ SDL_CONTROLLER_BUTTON_A, B },
		{ SDL_CONTROLLER_BUTTON_Y, X },
		{ SDL_CONTROLLER_BUTTON_X, Y },
		{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, R },
		{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER, L },
		{ SDL_CONTROLLER_BUTTON_START, START },
		{ SDL_CONTROLLER_BUTTON_BACK, SELECT },
		{ SDL_CONTROLLER_BUTTON_DPAD_LEFT, LEFT },
		{ SDL_CONTROLLER_BUTTON_DPAD_RIGHT, RIGHT },
		{ SDL_CONTROLLER_BUTTON_DPAD_UP, UP },
		{ SDL_CONTROLLER_BUTTON_DPAD_DOWN, DOWN },
	};
}

void
sdl_platform::handle_events()
{
	SDL_Event e;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_KEYDOWN:
			handle_key_event(e.key.keysym.sym, true);
			break;
		case SDL_KEYUP:
			handle_key_event(e.key.keysym.sym, false);
			break;
		case SDL_CONTROLLERDEVICEADDED:
			add_controller(e.cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			remove_controller(e.cdevice.which);
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		{
			handle_controller_button_event(e.cbutton.button, true);
			break;
		}
		case SDL_CONTROLLERBUTTONUP:
		{
			handle_controller_button_event(
					e.cbutton.button, false);
			break;
		}
		case SDL_WINDOWEVENT:
			switch (e.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				event_window_size_changed(e.window.data1,
						e.window.data2);
				break;
			}
			break;
		}
	}

	update_touchscreen_state();
}

void
sdl_platform::handle_key_event(SDL_Keycode key, bool down)
{
	auto it = key_map.find(key);
	if (!ctrl_down && it != key_map.end()) {
		nds->button_event(it->second, down);
		return;
	}

	switch (key) {
	case SDLK_LCTRL:
		ctrl_down = down;
		break;
	}

	if (!down) {
		return;
	}

	if (ctrl_down) {
		switch (key) {
		case SDLK_0:
			reset_window_size(sdl_config.window_scale);
			break;
		case SDLK_1:
			reset_window_size(1);
			break;
		case SDLK_2:
			reset_window_size(2);
			break;
		case SDLK_3:
			reset_window_size(3);
			break;
		case SDLK_4:
			reset_window_size(4);
			break;
		case SDLK_EQUALS:
			adjust_window_size(1);
			break;
		case SDLK_MINUS:
			adjust_window_size(-1);
			break;
		case SDLK_s:
			take_screenshot(nds->get_framebuffer());
			break;
		}
	} else {
		switch (key) {
		case SDLK_0:
			throttle = !throttle;
			break;
		case SDLK_f:
			toggle_fullscreen();
			break;
		}
	}
}

void
sdl_platform::handle_controller_button_event(int button, bool down)
{
	auto it = button_map.find(button);
	if (it != button_map.end()) {
		nds->button_event(it->second, down);
	}
}

void
sdl_platform::add_controller(int joystick_index)
{
	SDL_GameController *gc = SDL_GameControllerOpen(joystick_index);
	if (gc) {
		SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gc);
		SDL_JoystickID id = SDL_JoystickInstanceID(joystick);
		controllers.insert(id);
	}
}

void
sdl_platform::remove_controller(SDL_JoystickID id)
{
	controllers.erase(id);

	SDL_GameController *gc = SDL_GameControllerFromInstanceID(id);
	if (gc) {
		SDL_GameControllerClose(gc);
	}
}

void
sdl_platform::update_touchscreen_state()
{
	int touch_x, touch_y;
	u32 mouse_buttons = SDL_GetMouseState(&touch_x, &touch_y);

	int ratio_cmp = window_w * 384 - window_h * 256;
	if (ratio_cmp == 0) {
		touch_x = touch_x * 256 / window_w;
		touch_y = (touch_y - window_h / 2) * 192 / (window_h / 2);
	} else if (ratio_cmp < 0) {
		touch_x = touch_x * 256 / window_w;
		int h = window_w * 384 / 256;
		touch_y = (touch_y - window_h / 2) * 192 / (h / 2);
	} else {
		int w = window_h * 256 / 384;
		touch_x = (touch_x - (window_w - w) / 2) * 256 / w;
		touch_y = (touch_y - window_h / 2) * 192 / (window_h / 2);
	}

	nds->update_touchscreen_state(
			touch_x, touch_y, mouse_buttons & SDL_BUTTON_LEFT);
}

void
sdl_platform::update_rtc()
{
	using namespace std::chrono;

	auto tp = zoned_time{ current_zone(), system_clock::now() }
	                          .get_local_time();
	auto dp = floor<days>(tp);
	year_month_day date{ dp };
	hh_mm_ss time{ tp - dp };
	weekday wday{ dp };

	nds->update_real_time_clock((int)date.year(), (unsigned)date.month(),
			(unsigned)date.day(), wday.iso_encoding(),
			time.hours().count(), time.minutes().count(),
			time.seconds().count());
}

void
sdl_platform::take_screenshot(void *fb)
{
	using namespace std::chrono;

	auto const tp = zoned_time{ current_zone(), system_clock::now() }
	                                .get_local_time();

	std::string filename = std::format("twice_screenshot_{:%Y%m%d-%H%M%S}",
			floor<seconds>(tp));
	std::string suffix = ".png";

	int err = write_nds_bitmap_to_png(fb, filename + suffix);
	if (!err) {
		return;
	}
	if (err != EEXIST) {
		goto error;
	}

	for (int i = 1; i < 10; i++) {
		suffix = std::format("_({}).png", i);
		err = write_nds_bitmap_to_png(fb, filename + suffix);
		if (!err) {
			return;
		}
		if (err != EEXIST) {
			goto error;
		}
	}

error:
	std::cout << "screenshot failed\n";
}

void
sdl_platform::event_window_size_changed(int w, int h)
{
	window_w = w;
	window_h = h;

	if (scaled_texture) {
		int scale = std::min(w / NDS_FB_W, h / NDS_FB_H);
		if (scale == 0) {
			scale = 1;
		}

		if (scale != texture_scale) {
			SDL_DestroyTexture(scaled_texture);
			scaled_texture =
					create_scaled_texture(renderer, scale);
			if (!scaled_texture) {
				std::cerr << "create scaled texture failed\n";
			}
			texture_scale = scale;
		}
	}
}

void
sdl_platform::reset_window_size(int scale)
{
	SDL_SetWindowSize(window, NDS_FB_W * scale, NDS_FB_H * scale);
}

void
sdl_platform::adjust_window_size(int step)
{
	int w = window_w + NDS_FB_W * step;
	int h = window_h + NDS_FB_H * step;

	if (w < 0 || h < 0 || w > 16384 || h > 16384) {
		return;
	}

	SDL_SetWindowSize(window, w, h);
}

void
sdl_platform::toggle_fullscreen()
{
	u32 flags = SDL_GetWindowFlags(window);
	if (flags & SDL_WINDOW_FULLSCREEN) {
		SDL_SetWindowFullscreen(window, 0);
	} else {
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
}

} // namespace twice
