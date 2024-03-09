#include <chrono>
#include <cstring>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "libtwice/config.h"
#include "libtwice/exception.h"
#include "libtwice/nds/defs.h"
#include "libtwice/nds/game_db.h"
#include "libtwice/nds/machine.h"

#include "args.h"
#include "platform.h"

twice::arg_parser twice::parser;

std::u8string
char_to_u8_string(const std::string& s)
{
	const char *p = s.data();
	std::u8string r{ (const char8_t *)p, s.size() };
	return r;
}

static std::string cartridge_pathname;
static std::string data_dir;

twice::arg_parser::opt_list twice::arg_parser::options = {
	{ "1x", '1', 0 },
	{ "2x", '2', 0 },
	{ "3x", '3', 0 },
	{ "4x", '4', 0 },
	{ "boot", 'b', 1 },
	{ "filter", '\0', 1 },
	{ "frame-limit", '\0', 1 },
	{ "fullscreen", 'f', 0 },
	{ "help", 'h', 0 },
	{ "no-throttle", '\0', 0 },
	{ "save", 's', 1 },
	{ "verbose", 'v', 0 },
};

twice::arg_parser::valid_opt_arg_list twice::arg_parser::valid_option_args = {
	{ "boot", { "firmware", "direct" } },
	{ "filter", { "nearest", "linear" } },
};

static void
print_usage()
{
	const std::string usage = R"___(usage: twice [OPTION]... FILE

Emulation options:
  -b, --boot <mode>     Set the boot mode. Defaults to direct boot.
                        (mode = 'direct' | 'firmware')
  -s, --save <type>     Set the save type. Defaults to unknown.
                        (type = 'unknown' | 'none' | 'eeprom <size>' | 'flash <size>')

Other options:
  -1, -2, -3, -4        Scale the viewport by 1x, 2x, 3x, 4x.
  -f, --fullscreen      Start in fullscreen mode.
  --filter <mode>       Set the screen filtering mode. Defaults to linear.
                        (mode = 'nearest' | 'linear' )
  --frame-limit <N>     Quit after running <N> frames.
  --no-throttle         Start with throttling disabled.
  -v, --verbose         Use verbose output. Repeat to increase verbosity.
  -h, --help            Print this help.
)___";

	std::cerr << usage;
}

int
main(int argc, char **argv)
try {
	using twice::parser;
	twice::sdl_platform::config sdl_config;

	if (parser.parse_args(argc, argv)) {
		print_usage();
		return 1;
	}

	if (parser.get_option("help")) {
		print_usage();
		return 0;
	}

	if (parser.num_args() > 0) {
		cartridge_pathname = parser.get_arg(0).data();
	}
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

	twice::nds_save_info save_info{ twice::SAVETYPE_UNKNOWN, 0 };
	if (auto opt = parser.get_option("save")) {
		save_info.type = twice::nds_parse_savetype_string(opt->arg);
		save_info.size = twice::nds_savetype_to_size(save_info.type);
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

	if (parser.get_option("no-throttle")) {
		sdl_config.throttle = false;
	}

	if (auto opt = parser.get_option("frame-limit")) {
		twice::u64 frame_limit = 0;
		auto [ptr, ec] = std::from_chars(opt->arg.data(),
				opt->arg.data() + opt->arg.size(),
				frame_limit);
		if (ec == std::errc()) {
			sdl_config.frame_limit = frame_limit;
		} else {
			print_usage();
			return 1;
		}
	}

	bool direct_boot = true;
	if (auto opt = parser.get_option("boot")) {
		if (opt->arg == "firmware") {
			direct_boot = false;
		} else {
			direct_boot = true;
		}
	}
	if (direct_boot && cartridge_pathname.empty()) {
		print_usage();
		return 1;
	}

	if (data_dir.empty()) {
		data_dir = twice::get_data_dir();
		std::cerr << "data dir: " << data_dir << '\n';
	}

	twice::nds_config config;
	config.data_dir = char_to_u8_string(data_dir);
	twice::nds_machine nds(config);
	if (!cartridge_pathname.empty()) {
		nds.load_cartridge(char_to_u8_string(cartridge_pathname));
	}
	if (save_info.type != twice::SAVETYPE_UNKNOWN) {
		nds.set_savetype(save_info.type);
	}
	nds.boot(direct_boot);

	twice::sdl_platform platform(&nds, sdl_config);
	platform.loop();

	return EXIT_SUCCESS;
} catch (const twice::twice_exception& e) {
	std::cerr << "fatal error: " << e.what() << '\n';
	return EXIT_FAILURE;
}
