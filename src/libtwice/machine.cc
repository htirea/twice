#include "libtwice/machine.h"
#include "libtwice/exception.h"
#include "libtwice/nds.h"

using namespace twice;

Machine::Machine(Config& config)
	: config(config),
	  arm7_bios(config.data_dir + "bios7.bin", NDS_BIOS7_SIZE,
			  FileMap::MAP_EXACT_SIZE),
	  arm9_bios(config.data_dir + "bios9.bin", NDS_BIOS9_SIZE,
			  FileMap::MAP_EXACT_SIZE),
	  firmware(config.data_dir + "firmware.bin", NDS_FIRMWARE_SIZE,
			  FileMap::MAP_EXACT_SIZE)
{
}

int
Machine::load_cartridge(const std::string& pathname)
{
	try {
		cartridge = { pathname, NDS_MAX_CART_SIZE,
			FileMap::MAP_MAX_SIZE };
		return 0;
	} catch (TwiceFileError& e) {
		fprintf(stderr, "%s\n", e.what());
		return 1;
	}
}

void
Machine::run_frame()
{
	std::cerr << "run frame\n";
}
