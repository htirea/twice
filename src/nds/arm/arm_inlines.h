#ifndef TWICE_ARM_INLINES_H
#define TWICE_ARM_INLINES_H

#include "nds/arm/arm.h"

namespace twice {

inline void
arm_set_t(arm_cpu *cpu, bool t)
{
	cpu->cpsr = (cpu->cpsr & ~BIT(5)) | (t << 5);
}

inline bool
arm_in_thumb(arm_cpu *cpu)
{
	return cpu->cpsr & BIT(5);
}

inline void
arm_force_stop(arm_cpu *cpu)
{
	cpu->target_cycles = cpu->cycles;
}

inline void
arm_check_interrupt(arm_cpu *cpu)
{
	cpu->interrupt = !(cpu->cpsr & BIT(7)) && (cpu->IME & 1) &&
			(cpu->IE & cpu->IF);
}

inline void
arm_request_interrupt(arm_cpu *cpu, int bit)
{
	cpu->IF |= BIT(bit);
	arm_check_interrupt(cpu);
}

inline void
arm_do_irq(arm_cpu *cpu)
{
	u32 old_cpsr = cpu->cpsr;
	u32 ret_addr = cpu->pc() - (arm_in_thumb(cpu) ? 2 : 4) + 4;

	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x92;
	arm_switch_mode(cpu, MODE_IRQ);
	cpu->interrupt = false;

	cpu->gpr[14] = ret_addr;
	cpu->spsr() = old_cpsr;
	cpu->arm_jump(cpu->exception_base + 0x18);
}

inline bool
arm_check_cond(arm_cpu *cpu, u32 cond)
{
	if (cond == 0xE) {
		return true;
	}

	bool N = cpu->cpsr & BIT(31);
	bool Z = cpu->cpsr & BIT(30);
	bool C = cpu->cpsr & BIT(29);
	bool V = cpu->cpsr & BIT(28);

	switch (cond) {
	case 0x0:
		return Z;
	case 0x1:
		return !Z;
	case 0x2:
		return C;
	case 0x3:
		return !C;
	case 0x4:
		return N;
	case 0x5:
		return !N;
	case 0x6:
		return V;
	case 0x7:
		return !V;
	case 0x8:
		return C && !Z;
	case 0x9:
		return !C || Z;
	case 0xA:
		return N == V;
	case 0xB:
		return N != V;
	case 0xC:
		return !Z && N == V;
	case 0xD:
		return Z || N != V;
	}

	return false;
}

} // namespace twice

#endif
