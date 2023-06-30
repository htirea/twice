#ifndef TWICE_ARM_EXCEPTION_H
#define TWICE_ARM_EXCEPTION_H

#include "nds/arm/interpreter/util.h"

namespace twice {

inline void
arm_undefined(arm_cpu *cpu)
{
	u32 old_cpsr = cpu->cpsr;

	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x9B;
	switch_mode(cpu, MODE_UND);
	cpu->interrupt = false;

	cpu->gpr[14] = cpu->pc() - 4;
	cpu->spsr() = old_cpsr;
	cpu->arm_jump(cpu->exception_base + 0x4);
}

inline void
arm_swi(arm_cpu *cpu)
{
	u32 old_cpsr = cpu->cpsr;

	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x93;
	switch_mode(cpu, MODE_SVC);
	cpu->interrupt = false;

	cpu->gpr[14] = cpu->pc() - 4;
	cpu->spsr() = old_cpsr;
	cpu->arm_jump(cpu->exception_base + 0x8);
}

inline void
arm_bkpt(arm_cpu *cpu)
{
	(void)cpu;
	throw twice_error("arm bkpt not implemented");
}

} // namespace twice

#endif
