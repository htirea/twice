#include "libtwice/arm/arm.h"

using namespace twice;

Arm::Arm(NDS *nds)
	: nds(nds)
{
}

Arm9::Arm9(NDS *nds)
	: Arm(nds)
{
}

Arm7::Arm7(NDS *nds)
	: Arm(nds)
{
}

void
Arm9::jump(u32 addr)
{
}

void
Arm7::jump(u32 addr)
{
}

void
Arm9::cp15_write(u32 reg, u32 value)
{
}
