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

template <int A, int S>
void
arm_multiply(Arm *cpu)
{
	u32 rd = cpu->opcode >> 16 & 0xF;
	u32 rs = cpu->opcode >> 8 & 0xF;
	u32 rm = cpu->opcode & 0xF;

	u32 r = cpu->gpr[rm] * cpu->gpr[rs];

	if (A) {
		u32 rn = cpu->opcode >> 12 & 0xF;
		r += cpu->gpr[rn];
	}

	cpu->gpr[rd] = r;

	if (S) {
		cpu->set_nz(r >> 31, r == 0);
	}
}

template <int U, int A, int S>
void
arm_multiply_long(Arm *cpu)
{
	u32 rdhi = cpu->opcode >> 16 & 0xF;
	u32 rdlo = cpu->opcode >> 12 & 0xF;
	u32 rs = cpu->opcode >> 8 & 0xF;
	u32 rm = cpu->opcode & 0xF;

	u64 r;
	/* U is flipped on purpose! */
	if (U) {
		r = (s64)(s32)cpu->gpr[rm] * (s32)cpu->gpr[rs];
	} else {
		r = (u64)cpu->gpr[rm] * cpu->gpr[rs];
	}

	if (A) {
		r += (u64)cpu->gpr[rdhi] << 32 | cpu->gpr[rdlo];
	}

	cpu->gpr[rdhi] = r >> 32;
	cpu->gpr[rdlo] = r;

	if (S) {
		cpu->set_nz(r >> 63, r == 0);
	}
}

} // namespace twice

#endif
