#include "platform.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>

#include "screenshot.h"

namespace twice {

sdl_platform::sdl_platform(nds_machine *nds, const config& sdl_config)
	: sdl_config(sdl_config), nds(nds)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER |
			    SDL_INIT_AUDIO)) {
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

	textures[0] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
			SDL_TEXTUREACCESS_STREAMING, NDS_FB_W, NDS_FB_H);
	if (!textures[0]) {
		throw sdl_error("create texture failed");
	}
	if (SDL_SetTextureScaleMode(textures[0], sdl_config.scale_mode)) {
		throw sdl_error("set texture scale mode failed");
	}
	textures[1] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
			SDL_TEXTUREACCESS_STREAMING, NDS_FB_H, NDS_FB_W);
	if (!textures[1]) {
		throw sdl_error("create texture failed");
	}
	if (SDL_SetTextureScaleMode(textures[1], sdl_config.scale_mode)) {
		throw sdl_error("set texture scale mode failed");
	}

	SDL_AudioSpec want;
	SDL_memset(&want, 0, sizeof(want));
	want.freq = NDS_AUDIO_SAMPLE_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = NULL;
	audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &audio_spec, 0);
	if (!audio_dev) {
		throw sdl_error("create audio device failed");
	}
	SDL_PauseAudioDevice(audio_dev, 0);

	int num_joysticks = SDL_NumJoysticks();
	for (int i = 0; i < num_joysticks; i++) {
		if (SDL_IsGameController(i)) {
			add_controller(i);
		}
	}

	setup_default_binds();

	freq = SDL_GetPerformanceFrequency();
}

sdl_platform::~sdl_platform()
{
	while (!controllers.empty()) {
		SDL_JoystickID id = *controllers.begin();
		remove_controller(id);
	}

	if (audio_dev) {
		SDL_CloseAudioDevice(audio_dev);
	}
	SDL_DestroyTexture(textures[0]);
	SDL_DestroyTexture(textures[1]);
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

static void
copy_framebuffer_to_texture(u32 *dest, u32 *src, int orientation)
{
	switch (orientation) {
	case 0:
		std::memcpy(dest, src, NDS_FB_SZ_BYTES);
		break;
	case 1:
		for (u32 i = NDS_FB_H; i--;) {
			for (u32 j = 0; j < NDS_FB_W; j++) {
				dest[j * NDS_FB_H + i] = *src++;
			}
		}
		break;
	case 2:
		std::reverse_copy(src, src + NDS_FB_SZ, dest);
		break;
	case 3:
		for (u32 i = 0; i < NDS_FB_H; i++) {
			for (u32 j = NDS_FB_W; j--;) {
				dest[j * NDS_FB_H + i] = *src++;
			}
		}
	}
}

void
sdl_platform::render(u32 *fb)
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);

	int pitch;
	void *p;
	SDL_Texture *texture = textures[orientation & 1];
	SDL_LockTexture(texture, NULL, &p, &pitch);
	copy_framebuffer_to_texture((u32 *)p, fb, orientation);
	SDL_UnlockTexture(texture);

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, texture, NULL, NULL);

	SDL_RenderPresent(renderer);
}

void
sdl_platform::queue_audio(s16 *audiobuffer, u32 size, u64 ticks)
{
	static double acc = 0;
	double samples = (double)NDS_AUDIO_SAMPLE_RATE / NDS_FRAME_RATE;
	double samples_max = (double)NDS_AUDIO_SAMPLE_RATE * ticks / freq;
	double excess = samples - samples_max;

	if (std::abs(excess) < 0.1) {
		excess = 0;
	}

	if (excess < 0) {
		acc += excess;
	}

	if (excess > 0) {
		excess += acc;
		acc = 0;
	}

	if (excess > 0) {
		return;
	}

	SDL_QueueAudio(audio_dev, audiobuffer, size);
}

void
sdl_platform::arm_set_title(u64 ticks, std::pair<double, double> cpu_usage)
{
	double fps = (double)freq / ticks;
	double frametime = 1000.0 * ticks / freq;
	std::string title = std::format(
			"Twice [{:.2f} fps | {:.2f} ms | {:.2f} | {:.2f}]",
			fps, frametime, cpu_usage.first, cpu_usage.second);
	SDL_SetWindowTitle(window, title.c_str());
}

void
sdl_platform::loop()
{
	running = true;
	throttle = sdl_config.throttle;
	frames = 0;
	frame_limit = sdl_config.frame_limit;
	use_16_bit_audio = true;
	nds->set_use_16_bit_audio(true);

	u64 target = freq / NDS_FRAME_RATE;
	u64 last_elapsed = target;
	u64 total_elapsed = 0;
	u64 lag = 0;
	u64 emulation_start = SDL_GetPerformanceCounter();

	update_rtc();

	while (running) {
		u64 start = SDL_GetPerformanceCounter();

		total_elapsed += last_elapsed;
		if (total_elapsed >= freq) {
			total_elapsed -= freq;
			arm_set_title(fps_counter.get_average(),
					nds->get_cpu_usage());
		}
		fps_counter.insert(last_elapsed);

		handle_events();

		if (!paused || step_frame) {
			nds->run_frame();
			render(nds->get_framebuffer());
			step_frame = false;
			frames++;
			if (frames == frame_limit) {
				running = false;
			}
		}

		if (!paused && throttle && !audio_muted) {
			queue_audio(nds->get_audio_buffer(),
					nds->get_audio_buffer_size(),
					last_elapsed);
		}

		if (nds->is_shutdown()) {
			running = false;
		}

		u64 elapsed = SDL_GetPerformanceCounter() - start;
		u64 extra;

		if (elapsed >= target) {
			extra = 0;
			lag += elapsed - target;
		} else {
			extra = target - elapsed;
			u64 dt = std::min(extra, lag);
			extra -= dt;
			lag -= dt;
		}

		if (paused || (throttle && extra > 0)) {
			u64 end = start + elapsed + extra;
			u32 remaining_ms = extra * 1000 / freq;
			if (remaining_ms > 4) {
				SDL_Delay(remaining_ms - 4);
			}
			while (SDL_GetPerformanceCounter() < end)
				;
		}

		last_elapsed = SDL_GetPerformanceCounter() - start;
	}

	total_elapsed = SDL_GetPerformanceCounter() - emulation_start;
	std::cerr << std::format(
			"emulated {} frames in {} seconds, avg fps: {}\n",
			frames, (double)total_elapsed / freq,
			(double)frames * freq / total_elapsed);
	nds->dump_profiler_report();
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
		case SDLK_d:
			nds->dump_profiler_report();
			use_16_bit_audio = !use_16_bit_audio;
			nds->set_use_16_bit_audio(use_16_bit_audio);
			break;
		case SDLK_a:
			use_16_bit_audio = !use_16_bit_audio;
			nds->set_use_16_bit_audio(use_16_bit_audio);
			std::cerr << "use 16 bit audio: " << use_16_bit_audio
				  << '\n';
			break;
		case SDLK_LEFT:
			rotate_screen(-1);
			break;
		case SDLK_RIGHT:
			rotate_screen(1);
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
		case SDLK_m:
			audio_muted = !audio_muted;
			break;
		case SDLK_ESCAPE:
			paused = !paused;
			break;
		case SDLK_PERIOD:
			step_frame = true;
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

	switch (orientation) {
	case 1:
	{
		int touch_x_s = touch_x;
		touch_x = touch_y;
		touch_y = window_w - touch_x_s;
		break;
	}
	case 2:
		touch_x = window_w - touch_x;
		touch_y = window_h - touch_y;
		break;
	case 3:
	{
		int touch_x_s = touch_x;
		touch_x = window_h - touch_y;
		touch_y = touch_x_s;
		break;
	}
	}

	int w = window_w;
	int h = window_h;
	if (orientation & 1) {
		std::swap(w, h);
	}

	int ratio_cmp = w * 384 - h * 256;
	if (ratio_cmp == 0) {
		touch_x = touch_x * 256 / w;
		touch_y = (touch_y - h / 2) * 192 / (h / 2);
	} else if (ratio_cmp < 0) {
		touch_x = touch_x * 256 / w;
		int h_clipped = w * 384 / 256;
		touch_y = (touch_y - h / 2) * 192 / (h_clipped / 2);
	} else {
		int w_clipped = h * 256 / 384;
		touch_x = (touch_x - (w - w_clipped) / 2) * 256 / w_clipped;
		touch_y = (touch_y - h / 2) * 192 / (h / 2);
	}

	nds->update_touchscreen_state(touch_x, touch_y,
			mouse_buttons & SDL_BUTTON_LEFT, false, false);
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
}

void
sdl_platform::reset_window_size(int scale)
{
	int w = NDS_FB_W * scale;
	int h = NDS_FB_H * scale;
	if (orientation & 1) {
		std::swap(w, h);
	}

	SDL_SetWindowSize(window, w, h);
}

void
sdl_platform::adjust_window_size(int step)
{
	int w = NDS_FB_W * step;
	int h = NDS_FB_H * step;
	if (orientation & 1) {
		std::swap(w, h);
	}

	w += window_w;
	h += window_h;

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

void
sdl_platform::rotate_screen(int dir)
{
	orientation = (orientation + dir) & 3;
	if (orientation & 1) {
		SDL_RenderSetLogicalSize(renderer, NDS_FB_H, NDS_FB_W);
	} else {
		SDL_RenderSetLogicalSize(renderer, NDS_FB_W, NDS_FB_H);
	}

	if (dir & 1) {
		SDL_SetWindowSize(window, window_h, window_w);
	}
}

} // namespace twice
