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
	cpu->arm_jump(cpu->pc() + offset);
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

inline u32
arm_do_ldr(Arm *cpu, u32 addr)
{
	return std::rotr(cpu->load32(addr & ~3), (addr & 3) << 3);
}

inline void
arm_do_str(Arm *cpu, u32 addr, u32 value)
{
	cpu->store32(addr & ~3, value);
}

inline void
arm_do_strh(Arm *cpu, u32 addr, u16 value)
{
	cpu->store16(addr & ~1, value);
}

inline u8
arm_do_ldrb(Arm *cpu, u32 addr)
{
	return cpu->load8(addr);
}

inline void
arm_do_strb(Arm *cpu, u32 addr, u8 value)
{
	cpu->store8(addr, value);
}

inline s8
arm_do_ldrsb(Arm *cpu, u32 addr)
{
	return cpu->load8(addr);
}

template <int B>
void
arm_swap(Arm *cpu)
{
	u32 rn = cpu->opcode >> 16 & 0xF;
	u32 rd = cpu->opcode >> 12 & 0xF;
	u32 rm = cpu->opcode & 0xF;

	if (B) {
		u8 temp = arm_do_ldrb(cpu, cpu->gpr[rn]);
		arm_do_strb(cpu, cpu->gpr[rn], cpu->gpr[rm]);
		cpu->gpr[rd] = temp;
	} else {
		u32 temp = arm_do_ldr(cpu, cpu->gpr[rn]);
		arm_do_str(cpu, cpu->gpr[rn], cpu->gpr[rm]);
		cpu->gpr[rd] = temp;
	}
}

template <int P, int U, int I, int W, int L, int S, int H>
void
arm_misc_dt(Arm *cpu)
{
	u32 rn = cpu->opcode >> 16 & 0xF;
	u32 rd = cpu->opcode >> 12 & 0xF;
	u32 imm_hi = cpu->opcode >> 8 & 0xF;
	u32 imm_lo = cpu->opcode & 0xF;
	u32 rm = cpu->opcode & 0xF;

	u32 offset;
	u32 address;

	if (I) {
		offset = imm_hi << 4 | imm_lo;
	} else {
		offset = cpu->gpr[rm];
	}

	if (!U) {
		offset = -offset;
	}

	if (P) {
		address = cpu->gpr[rn] + offset;
	} else {
		address = cpu->gpr[rn];
	}

	constexpr bool writeback = W == 1 || P == 0;

	if (L == 1) {
		if (writeback) {
			cpu->gpr[rn] += offset;
		}

		u32 value;
		if (S == 0 && H == 1) {
			value = cpu->ldrh(address);
		} else if (S == 1 && H == 0) {
			value = arm_do_ldrsb(cpu, address);
		} else if (S == 1 && H == 1) {
			value = cpu->ldrsh(address);
		}

		if (rd == 15) {
			cpu->arm_jump(value & ~3);
		} else {
			cpu->gpr[rd] = value;
		}
	} else if (L == 0 && S == 0 && H == 1) {
		u16 value = cpu->gpr[rd];

		if (rd == 15) {
			value += 4;
		}

		arm_do_strh(cpu, address, value);

		if (writeback) {
			cpu->gpr[rn] += offset;
		}
	} else if (L == 0 && S == 1 && H == 0) {
		if (cpu->is_arm7()) {
			return;
		}

		if (rd & 1) {
			fprintf(stderr, "ldrd odd reg\n");
			rd &= ~1;
		}

		if (writeback) {
			cpu->gpr[rn] += offset;
		}

		cpu->gpr[rd] = cpu->load32(address & ~3);
		u32 value = cpu->load32((address & ~3) + 4);

		if (rd + 1 == 15) {
			cpu->arm_jump(value & ~3);
		} else {
			cpu->gpr[rd + 1] = value;
		}
	} else if (L == 0 && S == 1 && H == 1) {
		if (cpu->is_arm7()) {
			return;
		}

		if (rd & 1) {
			fprintf(stderr, "strd odd reg\n");
			rd &= ~1;
		}

		cpu->store32(address & ~3, cpu->gpr[rd]);
		u32 value = cpu->gpr[rd + 1];

		if (rd + 1 == 15) {
			value += 4;
		}

		cpu->store32((address & ~3) + 4, value);

		if (writeback) {
			cpu->gpr[rn] += offset;
		}
	}
}

template <int P, int U, int B, int W, int L, int SHIFT>
void
arm_sdt(Arm *cpu)
{
	u32 rn = cpu->opcode >> 16 & 0xF;
	u32 rd = cpu->opcode >> 12 & 0xF;

	u32 offset;
	u32 address;

	if (SHIFT == 4) {
		offset = cpu->opcode & 0xFFF;
	} else {
		u32 rm = cpu->opcode & 0xF;
		u32 shift_imm = cpu->opcode >> 7 & 0x1F;
		switch (SHIFT) {
		case 0:
			offset = cpu->gpr[rm] << shift_imm;
			break;
		case 1:
			if (shift_imm == 0) {
				offset = 0;
			} else {
				offset = cpu->gpr[rm] >> shift_imm;
			}
			break;
		case 2:
			if (shift_imm == 0) {
				if (cpu->gpr[rm] >> 31) {
					offset = 0xFFFFFFFF;
				} else {
					offset = 0;
				}
			} else {
				offset = (s32)cpu->gpr[rm] >> shift_imm;
			}
			break;
		default: /* case 3 */
			if (shift_imm == 0) {
				offset = (cpu->cpsr & (1 << 29)) << 2 |
						cpu->gpr[rm] >> 1;
			} else {
				offset = std::rotr(cpu->gpr[rm], shift_imm);
			}
		}
	}

	if (!U) {
		offset = -offset;
	}

	if (P) {
		address = cpu->gpr[rn] + offset;
	} else {
		address = cpu->gpr[rn];
	}

	constexpr bool writeback = W == 1 || P == 0;

	if (L == 1) {
		if (writeback) {
			cpu->gpr[rn] += offset;
		}

		u32 value;
		if (B == 1) {
			value = arm_do_ldrb(cpu, address);
		} else {
			value = arm_do_ldr(cpu, address);
		}

		if (rd == 15) {
			if (!cpu->is_arm7()) {
				arm_do_bx(cpu, value);
			} else {
				cpu->arm_jump(value & ~3);
			}
		} else {
			cpu->gpr[rd] = value;
		}
	} else {
		u32 value = cpu->gpr[rd];
		if (rd == 15) {
			value += 4;
		}

		if (B == 1) {
			arm_do_strb(cpu, address, value);
		} else {
			arm_do_str(cpu, address, value);
		}

		if (writeback) {
			cpu->gpr[rn] += offset;
		}
	}
}

} // namespace twice

#endif
