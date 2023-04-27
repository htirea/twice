#ifndef LIBTWICE_MACHINE_H
#define LIBTWICE_MACHINE_H

#include <cstddef>

namespace twice {

constexpr std::size_t NDS_FB_W = 256;
constexpr std::size_t NDS_FB_H = 384;
constexpr std::size_t NDS_FB_SZ = NDS_FB_W * NDS_FB_H;

struct Machine {
	void run_frame();
};

} // namespace twice

#endif
