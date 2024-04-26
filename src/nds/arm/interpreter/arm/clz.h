#ifndef TWICE_ARM_CLZ_H
#define TWICE_ARM_CLZ_H

#include "nds/arm/interpreter/arm/exception.h"
#include "nds/arm/interpreter/util.h"

namespace twice::arm::interpreter {

template <typename CPUT>
void
arm_clz(CPUT *cpu)
{
	if (is_arm7(cpu)) {
		return arm_undefined(cpu);
	}

	u32 rd = cpu->opcode >> 12 & 0xF;
	u32 rm = cpu->opcode & 0xF;
	cpu->gpr[rd] = std::countl_zero(cpu->gpr[rm]);

	cpu->add_code_cycles();
}

} // namespace twice::arm::interpreter

#endif
