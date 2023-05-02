#ifndef LIBTWICE_BUS_H
#define LIBTWICE_BUS_H

#include "libtwice/types.h"

namespace twice {

struct NDS;

template <typename T> T bus9_read(NDS *nds, u32 addr);
template <typename T> void bus9_write(NDS *nds, u32 addr, T value);
template <typename T> T bus7_read(NDS *nds, u32 addr);
template <typename T> void bus7_write(NDS *nds, u32 addr, T value);

} // namespace twice

#endif
