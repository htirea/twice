#ifndef TWICE_ARM_MULTIPLY_H
#define TWICE_ARM_MULTIPLY_H

#include "nds/arm/interpreter/util.h"

namespace twice::arm::interpreter {

template <typename CPUT, int A, int S>
void
arm_multiply(CPUT *cpu)
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
		set_nz(cpu, r >> 31, r == 0);
	}

	u32 icycles = 0;
	if (is_arm9(cpu)) {
		icycles = S ? 3 : 1;
	} else {
		icycles = get_mul_cycles(rs & BIT(31) ? ~rs : rs);
		icycles += A;
	}

	cpu->add_code_cycles(icycles);
}

template <typename CPUT, int U, int A, int S>
void
arm_multiply_long(CPUT *cpu)
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
		set_nz(cpu, r >> 63, r == 0);
	}

	u32 icycles = 0;
	if (is_arm9(cpu)) {
		icycles = S ? 4 : 2;
	} else {
		icycles = get_mul_cycles(U && rs & BIT(31) ? ~rs : rs);
		icycles += A + 1;
	}

	cpu->add_code_cycles(icycles);
}

} // namespace twice::arm::interpreter

#endif
