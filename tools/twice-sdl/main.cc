#include <iostream>

#include <SDL2/SDL.h>

#include <libtwice/machine.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

static bool running;

static void
print_usage()
{
	const std::string usage = "usage: twice [OPTION]... FILE\n";

	std::cerr << usage;
}

static int
parse_args(int argc, char **argv)
{
	return 0;
}

static int
init()
{
	using namespace twice;

	if (SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "sdl: init failed\n");
		return 1;
	}

	window = SDL_CreateWindow("Twice", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, NDS_FB_W * 2, NDS_FB_H * 2,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		fprintf(stderr, "sdl: create window failed\n");
		return 1;
	}

	SDL_SetWindowResizable(window, SDL_TRUE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		fprintf(stderr, "sdl: create renderer failed\n");
		return 1;
	}

	if (SDL_RenderSetLogicalSize(renderer, NDS_FB_W, NDS_FB_H)) {
		fprintf(stderr, "sdl: renderer set logical size failed\n");
		return 1;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING, NDS_FB_W, NDS_FB_H);
	if (!texture) {
		fprintf(stderr, "sdl: create texture failed\n");
		return 1;
	}

	return 0;
}

static void
destroy()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

static void
render()
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

static void
handle_events()
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

static void
loop(twice::Machine& nds)
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

	twice::Machine nds;

	if (init()) {
		std::cerr << "sdl init failed\n";
		destroy();
		return 1;
	}

	loop(nds);

	destroy();
	return 0;
}
