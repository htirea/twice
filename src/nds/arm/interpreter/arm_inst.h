#ifndef TWICE_ARM_INST_H
#define TWICE_ARM_INST_H

#include <nds/arm/arm.h>

#include <libtwice/exception.h>

namespace twice {

inline void
arm_noop(Arm *cpu)
{
}

inline void
arm_undefined(Arm *cpu)
{
	throw TwiceError("arm undefined instruction");
}

template <int L>
void
arm_b(Arm *cpu)
{
	if (L) {
		cpu->gpr[14] = cpu->pc() - 4;
	}

	s32 offset = (s32)(cpu->opcode << 8) >> 6;
	cpu->arm_jump(offset);
}

inline void
arm_do_bx(Arm *cpu, u32 addr)
{
	if (addr & 1) {
		cpu->set_t(1);
		cpu->thumb_jump(addr & ~1);
	} else {
		cpu->arm_jump(addr & ~3);
	}
}

inline void
arm_blx2(Arm *cpu)
{
	if (cpu->is_arm7()) {
		arm_undefined(cpu);
		return;
	}

	cpu->gpr[14] = cpu->pc() - 4;
	u32 addr = cpu->gpr[cpu->opcode & 0xF];
	arm_do_bx(cpu, addr);
}

inline void
arm_bx(Arm *cpu)
{
	u32 addr = cpu->gpr[cpu->opcode & 0xF];
	arm_do_bx(cpu, addr);
}

} // namespace twice

#endif
