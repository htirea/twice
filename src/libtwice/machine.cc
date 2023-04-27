#include "libtwice/machine.h"
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

void
Machine::run_frame()
{
	std::cerr << "run frame\n";
}
