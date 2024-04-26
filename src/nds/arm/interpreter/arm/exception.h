#ifndef TWICE_ARM_EXCEPTION_H
#define TWICE_ARM_EXCEPTION_H

#include "nds/arm/interpreter/util.h"

namespace twice::arm::interpreter {

template <typename CPUT>
void
arm_noop(CPUT *cpu)
{
	cpu->add_code_cycles();
}

template <typename CPUT>
void
arm_undefined(CPUT *cpu)
{
	u32 old_cpsr = cpu->cpsr;
	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x9B;
	arm_switch_mode(cpu, arm_cpu::MODE_UND);
	cpu->interrupt = false;
	cpu->gpr[14] = cpu->pc() - 4;
	cpu->spsr() = old_cpsr;

	cpu->add_code_cycles();
	cpu->arm_jump(cpu->exception_base + 0x4);
}

template <typename CPUT>
void
arm_swi(CPUT *cpu)
{
	u32 old_cpsr = cpu->cpsr;
	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x93;
	arm_switch_mode(cpu, arm_cpu::MODE_SVC);
	cpu->interrupt = false;
	cpu->gpr[14] = cpu->pc() - 4;
	cpu->spsr() = old_cpsr;

	cpu->add_code_cycles();
	cpu->arm_jump(cpu->exception_base + 0x8);
}

template <typename CPUT>
void
arm_bkpt(CPUT *)
{
	throw twice_error("arm bkpt not implemented");
}

} // namespace twice::arm::interpreter

#endif
