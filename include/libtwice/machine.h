#ifndef LIBTWICE_MACHINE_H
#define LIBTWICE_MACHINE_H

#include <cstddef>

#include "libtwice/config.h"
#include "libtwice/filemap.h"

namespace twice {

constexpr std::size_t NDS_FB_W = 256;
constexpr std::size_t NDS_FB_H = 384;
constexpr std::size_t NDS_FB_SZ = NDS_FB_W * NDS_FB_H;

struct Machine {
	Machine(Config& config);

	void run_frame();

      private:
	Config config;
	FileMap arm7_bios;
	FileMap arm9_bios;
	FileMap firmware;
};

} // namespace twice

#endif
