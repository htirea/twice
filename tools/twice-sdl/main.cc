#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include <SDL2/SDL.h>

#include <libtwice/exception.h>
#include <libtwice/machine.h>
#include <libtwice/types.h>

static std::string data_dir;
static std::string cartridge_pathname;

static void
print_usage()
{
	const std::string usage = "usage: twice [OPTION]... FILE\n";

	std::cerr << usage;
}

static int
parse_args(int argc, char **argv)
{
	if (argc - 1 < 1) {
		return 1;
	}

	cartridge_pathname = argv[1];

	return 0;
}

static void
set_data_dir()
{
	auto path = SDL_GetPrefPath("", "twice");
	if (path) {
		data_dir = path;
		SDL_free(path);
	}
}

struct sdl_exception : public std::exception {
	sdl_exception(const std::string& msg)
		: msg(msg)
	{
	}

	virtual const char *what() const noexcept override
	{
		return msg.c_str();
	}

      private:
	std::string msg;
};

struct Platform {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;

	bool running;
	uint64_t tstart;
	uint64_t tframe;
	uint64_t tfreq;
	uint64_t ticks_elapsed{};

	struct FpsCounter {
		static constexpr unsigned BUF_SIZE = 64;
		int buffer[BUF_SIZE]{};
		int sum{};
		int index{};

		void add(int fps)
		{
			sum += fps;
			sum -= buffer[index];
			buffer[index] = fps;
			index = (index + 1) % BUF_SIZE;
		}

		int get_average_fps() { return sum / BUF_SIZE; }
	} fpscounter;

	Platform();
	~Platform();
	void render(void *fb);
	void handle_events(twice::Machine& nds);
	void loop(twice::Machine& nds);
	void set_title_fps(int fps);
};

Platform::Platform()
{
	using namespace twice;

	if (SDL_Init(SDL_INIT_VIDEO)) {
		throw sdl_exception("init failed\n");
	}

	window = SDL_CreateWindow("Twice", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, NDS_FB_W * 2, NDS_FB_H * 2,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		throw sdl_exception("create window failed\n");
	}

	SDL_SetWindowResizable(window, SDL_TRUE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		throw sdl_exception("create renderer failed\n");
	}

	if (SDL_RenderSetLogicalSize(renderer, NDS_FB_W, NDS_FB_H)) {
		throw sdl_exception("renderer set logical size failed\n");
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING, NDS_FB_W, NDS_FB_H);
	if (!texture) {
		throw sdl_exception("create texture failed\n");
	}
}

Platform::~Platform()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
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
	std::string title = "Twice [" + std::to_string(fps) + "]";
	SDL_SetWindowTitle(window, title.c_str());
}

static twice::NdsButton
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
Platform::handle_events(twice::Machine& nds)
{
	SDL_Event e;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_KEYDOWN:
			nds.button_event(get_nds_button(e.key.keysym.sym), 1);
			break;
		case SDL_KEYUP:
			nds.button_event(get_nds_button(e.key.keysym.sym), 0);
			break;
		}
	}
}

void
Platform::loop(twice::Machine& nds)
{
	tfreq = SDL_GetPerformanceFrequency();
	tframe = tfreq / 60;
	tstart = SDL_GetPerformanceCounter();

	running = true;

	while (running) {
		handle_events(nds);
		nds.run_frame();
		render(nds.get_framebuffer());

		uint64_t elapsed = SDL_GetPerformanceCounter() - tstart;
		int fps = (double)tfreq / elapsed;
		fpscounter.add(fps);

		ticks_elapsed += elapsed;
		if (ticks_elapsed >= tfreq) {
			ticks_elapsed -= tfreq;
			set_title_fps(fpscounter.get_average_fps());
		}

		tstart = SDL_GetPerformanceCounter();
	}
}

int
main(int argc, char **argv)
{
	if (parse_args(argc, argv)) {
		print_usage();
		return 1;
	}

	if (data_dir.empty()) {
		set_data_dir();
		std::cerr << "data dir: " << data_dir << '\n';
	}

	twice::Config config{ data_dir };
	twice::Machine nds(config);

	nds.load_cartridge(cartridge_pathname);
	nds.direct_boot();

	Platform platform;

	platform.loop(nds);

	return 0;
}
