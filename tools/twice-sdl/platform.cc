#include "platform.h"

#include <cstring>
#include <format>

#include <libtwice/machine.h>
#include <libtwice/types.h>

namespace twice {

Platform::Platform()
{
	using namespace twice;

	if (SDL_Init(SDL_INIT_VIDEO)) {
		throw SDLError("init failed\n");
	}

	window = SDL_CreateWindow("Twice", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, NDS_FB_W * 2, NDS_FB_H * 2,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		throw SDLError("create window failed\n");
	}

	SDL_SetWindowResizable(window, SDL_TRUE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		throw SDLError("create renderer failed\n");
	}

	if (SDL_RenderSetLogicalSize(renderer, NDS_FB_W, NDS_FB_H)) {
		throw SDLError("renderer set logical size failed\n");
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888,
			SDL_TEXTUREACCESS_STREAMING, NDS_FB_W, NDS_FB_H);
	if (!texture) {
		throw SDLError("create texture failed\n");
	}
}

Platform::~Platform()
{
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
Platform::render(void *fb)
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);

	int pitch;
	void *p;
	SDL_LockTexture(texture, NULL, &p, &pitch);
	std::memcpy(p, fb, twice::NDS_FB_SZ_BYTES);
	SDL_UnlockTexture(texture);

	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void
Platform::set_title_fps(int fps)
{
	std::string title = std::format(
			"Twice [{} fps | {:.2f} ms]", fps, 1000.0 / fps);
	SDL_SetWindowTitle(window, title.c_str());
}

void
Platform::loop(twice::Machine *nds)
{
	uint64_t tfreq = SDL_GetPerformanceFrequency();
	uint64_t tstart = SDL_GetPerformanceCounter();
	uint64_t ticks_elapsed = 0;

	running = true;

	while (running) {
		handle_events(nds);
		nds->run_frame();
		render(nds->get_framebuffer());

		uint64_t elapsed = SDL_GetPerformanceCounter() - tstart;
		int fps = (double)tfreq / elapsed;

		fps_counter.add(fps);

		ticks_elapsed += elapsed;
		if (ticks_elapsed >= tfreq) {
			ticks_elapsed -= tfreq;
			set_title_fps(fps_counter.get_average_fps());
		}

		tstart = SDL_GetPerformanceCounter();
	}
}

static NdsButton
get_nds_button(SDL_Keycode key)
{
	using enum twice::NdsButton;

	switch (key) {
	case SDLK_x:
		return A;
	case SDLK_z:
		return B;
	case SDLK_s:
		return X;
	case SDLK_a:
		return Y;
	case SDLK_w:
		return R;
	case SDLK_q:
		return L;
	case SDLK_1:
		return START;
	case SDLK_2:
		return SELECT;
	case SDLK_LEFT:
		return LEFT;
	case SDLK_RIGHT:
		return RIGHT;
	case SDLK_UP:
		return UP;
	case SDLK_DOWN:
		return DOWN;
	default:
		return NONE;
	}
}

void
Platform::handle_events(twice::Machine *nds)
{
	SDL_Event e;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_KEYDOWN:
			nds->button_event(get_nds_button(e.key.keysym.sym), 1);
			break;
		case SDL_KEYUP:
			nds->button_event(get_nds_button(e.key.keysym.sym), 0);
			break;
		}
	}
}

} // namespace twice
