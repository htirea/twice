#ifndef TWICE_ARM_BRANCH_H
#define TWICE_ARM_BRANCH_H

#include "nds/arm/interpreter/arm/exception.h"
#include "nds/arm/interpreter/util.h"

namespace twice::arm::interpreter {

template <int L>
void
arm_b(arm_cpu *cpu)
{
	if (L) {
		cpu->gpr[14] = cpu->pc() - 4;
	}

	s32 offset = (s32)(cpu->opcode << 8) >> 6;

	cpu->add_code_cycles(0);
	cpu->arm_jump(cpu->pc() + offset);
}

inline void
arm_blx2(arm_cpu *cpu)
{
	if (is_arm7(cpu)) {
		arm_undefined(cpu);
		return;
	}

	u32 addr = cpu->gpr[cpu->opcode & 0xF];
	cpu->gpr[14] = cpu->pc() - 4;

	cpu->add_code_cycles(0);
	arm_do_bx(cpu, addr);
}

inline void
arm_bx(arm_cpu *cpu)
{
	u32 addr = cpu->gpr[cpu->opcode & 0xF];

	cpu->add_code_cycles(0);
	arm_do_bx(cpu, addr);
}

} // namespace twice::arm::interpreter

#endif
