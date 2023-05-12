#ifndef TWICE_THUMB_EXCEPTION_H
#define TWICE_THUMB_EXCEPTION_H

#include "nds/arm/interpreter/util.h"

namespace twice {

inline void
thumb_undefined(Arm *cpu)
{
	(void)cpu;
	throw TwiceError("thumb undefined instruction");
}

inline void
thumb_swi(Arm *cpu)
{
	u32 old_cpsr = cpu->cpsr;

	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x93;
	cpu->switch_mode(MODE_SVC);
	cpu->interrupt = false;

	cpu->gpr[14] = cpu->pc() - 2;
	cpu->spsr() = old_cpsr;
	cpu->arm_jump(cpu->exception_base + 0x8);
}

inline void
thumb_bkpt(Arm *cpu)
{
	(void)cpu;
	throw TwiceError("thumb bkpt not implemented");
}

} // namespace twice

#endif
