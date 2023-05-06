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

inline void
arm_clz(Arm *cpu)
{
	if (cpu->is_arm7()) {
		arm_undefined(cpu);
		return;
	}

	u32 rd = cpu->opcode >> 12 & 0xF;
	u32 rm = cpu->opcode & 0xF;

	cpu->gpr[rd] = std::countl_zero(cpu->gpr[rm]);
}

template <int R>
void
arm_mrs(Arm *cpu)
{
	u32 rd = cpu->opcode >> 12 & 0xF;

	if (R) {
		cpu->gpr[rd] = cpu->spsr();
	} else {
		cpu->gpr[rd] = cpu->cpsr;
	}
}

template <int I, int R>
void
arm_msr(Arm *cpu)
{
	u32 operand;
	if (I) {
		u32 imm = cpu->opcode & 0xFF;
		u32 rotate_imm = cpu->opcode >> 8 & 0xF;
		operand = std::rotr(imm, rotate_imm << 1);
	} else {
		u32 rm = cpu->opcode & 0xF;
		operand = cpu->gpr[rm];
	}

	u32 field_mask = cpu->opcode >> 16 & 0xF;
	u32 write_mask = 0;

	if (R == 0) {
		if (cpu->in_privileged_mode()) {
			for (int i = 0; i < 3; i++) {
				if (field_mask & (1 << i)) {
					write_mask |= 0xFF << 8 * i;
				}
			}
		}
		if (field_mask & (1 << 3)) {
			write_mask |= 0xFF << 24;
		}

		cpu->cpsr = (cpu->cpsr & ~write_mask) | (operand & write_mask);
		cpu->on_cpsr_write();

		if (operand & write_mask & (1 << 5)) {
			throw TwiceError("msr changed thumb bit");
		}
	} else {
		if (cpu->current_mode_has_spsr()) {
			for (int i = 0; i < 4; i++) {
				if (field_mask & (1 << i)) {
					write_mask |= 0xFF << 8 * i;
				}
			}

			cpu->spsr() = (cpu->spsr() & ~write_mask) |
					(operand & write_mask);
		}
	}
}

inline void
arm_swi(Arm *cpu)
{
	u32 old_cpsr = cpu->cpsr;

	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x93;
	cpu->switch_mode(MODE_SVC);

	/* TODO: disable interrupts */

	cpu->gpr[14] = cpu->pc() - 4;
	cpu->spsr() = old_cpsr;
	cpu->arm_jump(cpu->exception_base + 0x8);
}

inline void
arm_bkpt(Arm *cpu)
{
	throw TwiceError("arm bkpt not implemented");
}

inline u32
saturated_add(Arm *cpu, u32 a, u32 b)
{
	u32 r = a + b;

	if (((a ^ r) & (b ^ r)) >> 31) {
		r = r >> 31 ? 0x7FFFFFFF : 0x80000000;
		cpu->set_q(1);
	}

	return r;
}

inline u32
saturated_sub(Arm *cpu, u32 a, u32 b)
{
	u32 r = a - b;

	if (((a ^ b) & (a ^ r)) >> 31) {
		r = r >> 31 ? 0x7FFFFFFF : 0x80000000;
		cpu->set_q(1);
	}

	return r;
}

template <int OP>
void
arm_sat_add_sub(Arm *cpu)
{
	if (cpu->is_arm7()) {
		arm_undefined(cpu);
		return;
	}

	u32 rn = cpu->opcode >> 16 & 0xF;
	u32 rd = cpu->opcode >> 12 & 0xF;
	u32 rm = cpu->opcode & 0xF;

	switch (OP) {
	case 0:
		cpu->gpr[rd] = saturated_add(cpu, cpu->gpr[rm], cpu->gpr[rn]);
		break;
	case 1:
		cpu->gpr[rd] = saturated_sub(cpu, cpu->gpr[rm], cpu->gpr[rn]);
		break;
	case 2:
	{
		u32 double_rn = saturated_add(cpu, cpu->gpr[rn], cpu->gpr[rn]);
		cpu->gpr[rd] = saturated_add(cpu, cpu->gpr[rm], double_rn);
		break;
	}
	case 3:
	{
		u32 double_rn = saturated_add(cpu, cpu->gpr[rn], cpu->gpr[rn]);
		cpu->gpr[rd] = saturated_sub(cpu, cpu->gpr[rm], double_rn);
		break;
	}
	}
}

inline u32
sticky_add(Arm *cpu, u32 a, u32 b)
{
	u32 r = a + b;

	if (((a ^ r) & (b ^ r)) >> 31) {
		cpu->set_q(1);
	}

	return r;
}

template <int OP, int Y, int X>
void
arm_dsp_multiply(Arm *cpu)
{
	if (cpu->is_arm7()) {
		return;
	}

	u32 rd = cpu->opcode >> 16 & 0xF;
	u32 rn = cpu->opcode >> 12 & 0xF;
	u32 rs = cpu->opcode >> 8 & 0xF;
	u32 rm = cpu->opcode & 0xF;

	s32 a = X == 0 ? (s32)(cpu->gpr[rm] << 16) >> 16
		       : (s32)(cpu->gpr[rm]) >> 16;
	s32 b = Y == 0 ? (s32)(cpu->gpr[rs] << 16) >> 16
		       : (s32)(cpu->gpr[rs]) >> 16;

	switch (OP) {
	case 0x0:
		cpu->gpr[rd] = sticky_add(cpu, a * b, cpu->gpr[rn]);
		break;
	case 0x1:
	{
		u32 r = (s64)(s32)cpu->gpr[rm] * b >> 16;
		if (X) {
			cpu->gpr[rd] = r;
		} else {
			cpu->gpr[rd] = sticky_add(cpu, r, cpu->gpr[rn]);
		}

		break;
	}
	case 0x2:
	{
		u64 r = (s64)(a * b);
		r += (u64)cpu->gpr[rd] << 32 | cpu->gpr[rn];
		cpu->gpr[rd] = r >> 32;
		cpu->gpr[rn] = r;
		break;
	}
	case 0x3:
		cpu->gpr[rd] = a * b;
		break;
	}
}

} // namespace twice

#endif
