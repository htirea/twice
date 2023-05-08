#ifndef TWICE_MACHINE_H
#define TWICE_MACHINE_H

#include <cstddef>
#include <memory>
#include <string>

#include "libtwice/config.h"
#include "libtwice/filemap.h"

namespace twice {

struct NDS;

struct Machine {
	Machine(Config& config);
	~Machine();

	void load_cartridge(const std::string& pathname);
	void direct_boot();
	void run_frame();
	void *get_framebuffer();

      private:
	Config config;
	FileMap arm7_bios;
	FileMap arm9_bios;
	FileMap firmware;
	FileMap cartridge;
	std::unique_ptr<NDS> nds;
};

} // namespace twice

#endif
