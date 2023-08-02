#include <chrono>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "libtwice/exception.h"
#include "libtwice/machine.h"
#include "libtwice/nds_defs.h"

#include "args.h"
#include "platform.h"

twice::arg_parser twice::parser;
twice::sdl_platform_config twice::sdl_config;

static std::string cartridge_pathname;
static std::string data_dir;

twice::arg_parser::opt_list twice::arg_parser::options = {
	{ "1x", '1', 0 },
	{ "2x", '2', 0 },
	{ "3x", '3', 0 },
	{ "4x", '4', 0 },
	{ "filter", '\0', 1 },
	{ "fullscreen", 'f', 0 },
	{ "help", 'h', 0 },
	{ "verbose", 'v', 0 },
};

twice::arg_parser::valid_opt_arg_list twice::arg_parser::valid_option_args = {
	{ "verbose", { "nearest", "linear" } },
};

static void
print_usage()
{
	const std::string usage = "usage: twice [OPTION]... FILE\n";

	std::cerr << usage;
}

int
main(int argc, char **argv)
try {
	using twice::parser;
	using twice::sdl_config;

	if (parser.parse_args(argc, argv)) {
		print_usage();
		return 1;
	}

	if (parser.get_option("help")) {
		print_usage();
		return 0;
	}

	if (parser.num_args() == 0) {
		print_usage();
		return 1;
	}

	cartridge_pathname = parser.get_arg(0);
	if (auto opt = parser.get_option("verbose")) {
		twice::set_logger_verbose_level(opt->count);
	}

	if (auto opt = parser.get_option("filter")) {
		if (opt->arg == "nearest") {
			sdl_config.scale_mode = SDL_ScaleModeNearest;
		} else if (opt->arg == "linear") {
			sdl_config.scale_mode = SDL_ScaleModeLinear;
		}
	}

	if (parser.get_option("1x")) {
		sdl_config.window_scale = 1;
	}
	if (parser.get_option("2x")) {
		sdl_config.window_scale = 2;
	}
	if (parser.get_option("3x")) {
		sdl_config.window_scale = 3;
	}
	if (parser.get_option("4x")) {
		sdl_config.window_scale = 4;
	}
	if (parser.get_option("fullscreen")) {
		sdl_config.fullscreen = true;
	}

	if (data_dir.empty()) {
		data_dir = twice::get_data_dir();
		std::cerr << "data dir: " << data_dir << '\n';
	}

	twice::nds_config config{ data_dir };
	twice::nds_machine nds(config);
	nds.load_cartridge(cartridge_pathname);
	nds.direct_boot();

	twice::sdl_platform platform;
	platform.loop(&nds);

	return EXIT_SUCCESS;
} catch (const twice::twice_exception& e) {
	std::cerr << "fatal error: " << e.what() << '\n';
	return EXIT_FAILURE;
}
