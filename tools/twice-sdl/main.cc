#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include <SDL2/SDL.h>

#include <libtwice/exception.h>
#include <libtwice/machine.h>

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

	Platform();
	~Platform();
	void render();
	void handle_events();
	void loop(twice::Machine& nds);
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
Platform::render()
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void
Platform::handle_events()
{
	SDL_Event e;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			running = false;
			break;
		}
	}
}

void
Platform::loop(twice::Machine& nds)
{
	running = true;

	while (running) {
		handle_events();
		nds.run_frame();
		render();
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

	// platform.loop(nds);

	return 0;
}
