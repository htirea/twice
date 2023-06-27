#ifndef TWICE_ARM_CLZ_H
#define TWICE_ARM_CLZ_H

#include "nds/arm/interpreter/arm/exception.h"
#include "nds/arm/interpreter/util.h"

namespace twice {

inline void
arm_clz(arm_cpu *cpu)
{
	if (cpu->is_arm7()) {
		arm_undefined(cpu);
		return;
	}

	u32 rd = cpu->opcode >> 12 & 0xF;
	u32 rm = cpu->opcode & 0xF;

	cpu->gpr[rd] = std::countl_zero(cpu->gpr[rm]);
}

} // namespace twice

#endif
