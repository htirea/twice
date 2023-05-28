#include <chrono>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string>

#include <libtwice/exception.h>
#include <libtwice/machine.h>
#include <libtwice/types.h>

#include "platform.h"

static std::string cartridge_pathname;
static std::string data_dir;

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

int
main(int argc, char **argv)
{
	if (parse_args(argc, argv)) {
		print_usage();
		return 1;
	}

	if (data_dir.empty()) {
		data_dir = twice::get_data_dir();
		std::cerr << "data dir: " << data_dir << '\n';
	}

	twice::Config config{ data_dir };
	twice::Machine nds(config);

	nds.load_cartridge(cartridge_pathname);
	nds.direct_boot();

	twice::Platform platform;

	platform.loop(&nds);

	return 0;
}
