#include "libtwice/bus.h"
#include "libtwice/nds.h"

namespace twice {

template <typename T>
T
bus9_read(NDS *nds, u32 addr)
{
	return 0;
}

template <typename T>
void
bus9_write(NDS *nds, u32 addr, T value)
{
}

template <typename T>
T
bus7_read(NDS *nds, u32 addr)
{
	return 0;
}

template <typename T>
void
bus7_write(NDS *nds, u32 addr, T value)
{
}

template u32 bus9_read<u32>(NDS *nds, u32 addr);
template u16 bus9_read<u16>(NDS *nds, u32 addr);
template u8 bus9_read<u8>(NDS *nds, u32 addr);
template void bus9_write<u32>(NDS *nds, u32 addr, u32 value);
template void bus9_write<u16>(NDS *nds, u32 addr, u16 value);
template void bus9_write<u8>(NDS *nds, u32 addr, u8 value);

template u32 bus7_read<u32>(NDS *nds, u32 addr);
template u16 bus7_read<u16>(NDS *nds, u32 addr);
template u8 bus7_read<u8>(NDS *nds, u32 addr);
template void bus7_write<u32>(NDS *nds, u32 addr, u32 value);
template void bus7_write<u16>(NDS *nds, u32 addr, u16 value);
template void bus7_write<u8>(NDS *nds, u32 addr, u8 value);

} // namespace twice
