#include "libtwice/arm/arm7.h"
#include "libtwice/mem/bus.h"

using namespace twice;

void
Arm7::jump(u32 addr)
{
	if (in_thumb()) {
		pc() = addr + 2;
		pipeline[0] = fetch16(addr);
		pipeline[1] = fetch16(addr + 2);
	} else {
		pc() = addr + 4;
		pipeline[0] = fetch32(addr);
		pipeline[1] = fetch32(addr + 4);
	}
}

u32
Arm7::fetch32(u32 addr)
{
	return bus7_read<u32>(nds, addr);
}

u16
Arm7::fetch16(u32 addr)
{
	return bus7_read<u16>(nds, addr);
}

u32
Arm7::load32(u32 addr)
{
	return bus7_read<u32>(nds, addr);
}

u16
Arm7::load16(u32 addr)
{
	return bus7_read<u16>(nds, addr);
}

u8
Arm7::load8(u32 addr)
{
	return bus7_read<u8>(nds, addr);
}

void
Arm7::store32(u32 addr, u32 value)
{
	bus7_write<u32>(nds, addr, value);
}

void
Arm7::store16(u32 addr, u16 value)
{
	bus7_write<u16>(nds, addr, value);
}

void
Arm7::store8(u32 addr, u8 value)
{
	bus7_write<u8>(nds, addr, value);
}
