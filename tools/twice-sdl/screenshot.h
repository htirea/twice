#ifndef TWICE_SDL_SCREENSHOT_H
#define TWICE_SDL_SCREENSHOT_H

#include <string>

namespace twice {

constexpr int SCREENSHOT_FILE_EXISTS = 2;

int write_nds_bitmap_to_png(void *fb, const std::string& filename);

}

#endif
