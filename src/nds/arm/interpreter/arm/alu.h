#ifndef TWICE_ARM_ALU_H
#define TWICE_ARM_ALU_H

#include "nds/arm/interpreter/util.h"

namespace twice {

template <int OP, int S, int SHIFT, int MODE>
void
arm_alu(Arm *cpu)
{
	u32 operand;
	bool carry;

	if (MODE == 0) {
		u32 imm = cpu->opcode & 0xFF;
		u32 rotate_imm = cpu->opcode >> 8 & 0xF;

		operand = std::rotr(imm, rotate_imm << 1);
		carry = rotate_imm == 0 ? cpu->get_c() : operand >> 31;
	} else if (MODE == 1) {
		u32 rm = cpu->gpr[cpu->opcode & 0xF];
		u32 shift_imm = cpu->opcode >> 7 & 0x1F;

		switch (SHIFT) {
		case 0:
			if (shift_imm == 0) {
				operand = rm;
				carry = cpu->get_c();
			} else {
				operand = rm << shift_imm;
				carry = rm & BIT(32 - shift_imm);
			}
			break;
		case 1:
			if (shift_imm == 0) {
				operand = 0;
				carry = rm >> 31;
			} else {
				operand = rm >> shift_imm;
				carry = rm & BIT(shift_imm - 1);
			}
			break;
		case 2:
			if (shift_imm == 0) {
				if (rm >> 31 == 0) {
					operand = 0;
				} else {
					operand = 0xFFFFFFFF;
				}
				carry = rm >> 31;
			} else {
				operand = (s32)rm >> shift_imm;
				carry = rm & BIT(shift_imm - 1);
			}
			break;
		default:
			if (shift_imm == 0) {
				operand = cpu->get_c() << 31 | rm >> 1;
				carry = rm & 1;
			} else {
				operand = std::rotr(rm, shift_imm);
				carry = rm & BIT(shift_imm - 1);
			}
		}
	} else {
		u32 rmi = cpu->opcode & 0xF;
		u32 rm = cpu->gpr[rmi];
		u32 rs = cpu->gpr[cpu->opcode >> 8 & 0xF] & 0xFF;

		if (rmi == 15) {
			rm += 4;
		}

		switch (SHIFT) {
		case 0:
			if (rs == 0) {
				operand = rm;
				carry = cpu->get_c();
			} else if (rs < 32) {
				operand = rm << rs;
				carry = rm & BIT(32 - rs);
			} else if (rs == 32) {
				operand = 0;
				carry = rm & 1;
			} else {
				operand = 0;
				carry = 0;
			}
			break;
		case 1:
			if (rs == 0) {
				operand = rm;
				carry = cpu->get_c();
			} else if (rs < 32) {
				operand = rm >> rs;
				carry = rm & BIT(rs - 1);
			} else if (rs == 0) {
				operand = 0;
				carry = rm >> 31;
			} else {
				operand = 0;
				carry = 0;
			}
			break;
		case 2:
			if (rs == 0) {
				operand = rm;
				carry = cpu->get_c();
			} else if (rs < 32) {
				operand = (s32)rm >> rs;
				carry = rm & BIT(rs - 1);
			} else {
				if (rm >> 31 == 0) {
					operand = 0;
				} else {
					operand = 0xFFFFFFFF;
				}
				carry = rm >> 31;
			}
			break;
		default:
			if (rs == 0) {
				operand = rm;
				carry = cpu->get_c();
			} else if ((rs & 0x1F) == 0) {
				operand = rm;
				carry = rm >> 31;
			} else {
				operand = std::rotr(rm, rs & 0x1F);
				carry = rm & BIT((rs & 0x1F) - 1);
			}
		}
	}

	u32 rni = cpu->opcode >> 16 & 0xF;
	u32 rn = cpu->gpr[rni];

	if (MODE == 2 && rni == 15) {
		rn += 4;
	}

	u32 rd = cpu->opcode >> 12 & 0xF;

	u32 r;
	bool overflow;

	enum {
		AND,
		EOR,
		SUB,
		RSB,
		ADD,
		ADC,
		SBC,
		RSC,
		TST,
		TEQ,
		CMP,
		CMN,
		ORR,
		MOV,
		BIC,
		MVN
	};

	switch (OP) {
	case AND:
		r = rn & operand;
		break;
	case EOR:
		r = rn ^ operand;
		break;
	case SUB:
		r = rn - operand;
		SUB_FLAGS_(rn, operand);
		break;
	case RSB:
		r = operand - rn;
		SUB_FLAGS_(operand, rn);
		break;
	case ADD:
		r = rn + operand;
		ADD_FLAGS_(rn, operand);
		break;
	case ADC:
	{
		u64 r64 = (u64)rn + operand + cpu->get_c();
		r = r64;
		ADC_FLAGS_(rn, operand);
		break;
	}
	case SBC:
	{
		s64 r64 = (s64)rn - operand - !cpu->get_c();
		r = r64;
		SBC_FLAGS_(rn, operand);
		break;
	}
	case RSC:
	{
		s64 r64 = (s64)operand - rn - !cpu->get_c();
		r = r64;
		SBC_FLAGS_(operand, rn);
		break;
	}
	case TST:
		r = rn & operand;
		break;
	case TEQ:
		r = rn ^ operand;
		break;
	case CMP:
		r = rn - operand;
		SUB_FLAGS_(rn, operand);
		break;
	case CMN:
		r = rn + operand;
		ADD_FLAGS_(rn, operand);
		break;
	case ORR:
		r = rn | operand;
		break;
	case MOV:
		r = operand;
		break;
	case BIC:
		r = rn & ~operand;
		break;
	case MVN:
		r = ~operand;
		break;
	}

	if (OP == TST || OP == TEQ || OP == CMP || OP == CMN) {
		if (S && rd == 15) {
			cpu->cpsr = cpu->spsr();
			cpu->on_cpsr_write();
		}
	} else {
		if (S && rd == 15) {
			cpu->jump_cpsr(r);
		} else if (rd == 15) {
			cpu->arm_jump(r & ~3);
		} else {
			cpu->gpr[rd] = r;
		}
	}

	if (S && rd != 15) {
		if (OP == AND || OP == EOR || OP == TST || OP == TEQ ||
				OP == ORR || OP == MOV || OP == BIC ||
				OP == MVN) {
			cpu->set_nzc(r >> 31, r == 0, carry);
		} else {
			cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
		}
	}
}

} // namespace twice

#endif
