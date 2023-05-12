#ifndef TWICE_THUMB_ALU_H
#define TWICE_THUMB_ALU_H

#include "nds/arm/interpreter/util.h"

namespace twice {

template <int OP, int IMM>
void
thumb_alu1_2(Arm *cpu)
{
	u32 rn = cpu->gpr[cpu->opcode >> 3 & 0x7];
	u32 rd = cpu->opcode & 0x7;
	u32 rm = cpu->gpr[IMM];

	u32 r;
	bool carry;
	bool overflow;

	if (OP == 0) {
		r = rn + rm;
		ADD_FLAGS_(rn, rm);
	} else if (OP == 1) {
		r = rn - rm;
		SUB_FLAGS_(rn, rm);
	} else if (OP == 2) {
		r = rn + IMM;
		ADD_FLAGS_(rn, IMM);
	} else {
		r = rn - IMM;
		SUB_FLAGS_(rn, IMM);
	}

	cpu->gpr[rd] = r;
	cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
}

template <int OP, int RD>
void
thumb_alu3(Arm *cpu)
{
	u32 rn = cpu->gpr[RD];
	u32 imm = cpu->opcode & 0xFF;

	u32 r;
	bool carry;
	bool overflow;

	if (OP == 0) {
		cpu->gpr[RD] = r = imm;
	} else if (OP == 1) {
		r = rn - imm;
		SUB_FLAGS_(rn, imm);
	} else if (OP == 2) {
		cpu->gpr[RD] = r = rn + imm;
		ADD_FLAGS_(rn, imm);
	} else if (OP == 3) {
		cpu->gpr[RD] = r = rn - imm;
		SUB_FLAGS_(rn, imm);
	}

	if (OP == 0) {
		cpu->set_nz(r >> 31, r == 0);
	} else {
		cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
	}
}

template <int OP, int IMM>
void
thumb_alu4(Arm *cpu)
{
	u32 rm = cpu->gpr[cpu->opcode >> 3 & 0x7];
	u32 rd = cpu->opcode & 0x7;

	u32 r;
	bool carry;

	switch (OP) {
	case 0:
		if (IMM == 0) {
			r = rm;
			cpu->set_nz(r >> 31, r == 0);
		} else {
			r = rm << IMM;
			cpu->set_nzc(r >> 31, r == 0, rm & (1 << (32 - IMM)));
		}
		break;
	case 1:
		if (IMM == 0) {
			r = 0;
			carry = rm >> 31;
		} else {
			r = rm >> IMM;
			carry = rm & (1 << (IMM - 1));
		}
		cpu->set_nzc(r >> 31, r == 0, carry);
		break;
	case 2:
		if (IMM == 0) {
			carry = rm >> 31;
			if (rm >> 31 == 0) {
				r = 0;
			} else {
				r = 0xFFFFFFFF;
			}
		} else {
			r = (s32)rm >> IMM;
			carry = rm & (1 << (IMM - 1));
		}
		cpu->set_nzc(r >> 31, r == 0, carry);
		break;
	}

	cpu->gpr[rd] = r;
}

template <int OP>
void
thumb_alu5(Arm *cpu)
{
	enum {
		AND,
		EOR,
		LSL,
		LSR,
		ASR,
		ADC,
		SBC,
		ROR,
		TST,
		NEG,
		CMP,
		CMN,
		ORR,
		MUL,
		BIC,
		MVN,
	};

	u32 rm = cpu->gpr[cpu->opcode >> 3 & 0x7];
	u32 rd = cpu->opcode & 0x7;
	u32 operand = cpu->gpr[rd];
	u32 r;
	bool carry;
	bool overflow;

	switch (OP) {
	case AND:
		r = cpu->gpr[rd] = operand & rm;
		cpu->set_nz(r >> 31, r == 0);
		break;
	case EOR:
		r = cpu->gpr[rd] = operand ^ rm;
		cpu->set_nz(r >> 31, r == 0);
		break;
	case LSL:
		rm &= 0xFF;
		if (rm == 0) {
			r = operand;
			cpu->set_nz(r >> 31, r == 0);
		} else if (rm < 32) {
			carry = operand & (1 << (32 - rm));
			r = cpu->gpr[rd] = operand << rm;
			cpu->set_nzc(r >> 31, r == 0, carry);
		} else if (rm == 32) {
			carry = operand & 1;
			r = cpu->gpr[rd] = 0;
			cpu->set_nzc(r >> 31, r == 0, carry);
		} else {
			carry = 0;
			r = cpu->gpr[rd] = 0;
			cpu->set_nzc(r >> 31, r == 0, carry);
		}
		break;
	case LSR:
		rm &= 0xFF;
		if (rm == 0) {
			r = operand;
			cpu->set_nz(r >> 31, r == 0);
		} else if (rm < 32) {
			carry = operand & (1 << (rm - 1));
			r = cpu->gpr[rd] = operand >> rm;
			cpu->set_nzc(r >> 31, r == 0, carry);
		} else if (rm == 32) {
			carry = operand >> 31;
			r = cpu->gpr[rd] = 0;
			cpu->set_nzc(r >> 31, r == 0, carry);
		} else {
			carry = 0;
			r = cpu->gpr[rd] = 0;
			cpu->set_nzc(r >> 31, r == 0, carry);
		}
		break;
	case ASR:
		rm &= 0xFF;
		if (rm == 0) {
			r = operand;
			cpu->set_nz(r >> 31, r == 0);
		} else if (rm < 32) {
			carry = operand & (1 << (rm - 1));
			r = cpu->gpr[rd] = (s32)operand >> rm;
			cpu->set_nzc(r >> 31, r == 0, carry);
		} else {
			carry = operand >> 31;
			if (operand >> 31 == 0) {
				r = cpu->gpr[rd] = 0;
			} else {
				r = cpu->gpr[rd] = 0xFFFFFFFF;
			}
			cpu->set_nzc(r >> 31, r == 0, carry);
		}
		break;
	case ADC:
	{
		u64 r64 = operand + rm + cpu->get_c();
		r = cpu->gpr[rd] = r64;
		ADC_FLAGS_(operand, rm);
		cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
		break;
	}
	case SBC:
	{
		s64 r64 = operand - rm - !cpu->get_c();
		r = cpu->gpr[rd] = r64;
		SBC_FLAGS_(operand, rm);
		cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
		break;
	}
	case ROR:
		rm &= 0xFF;
		if (rm == 0) {
			r = operand;
			cpu->set_nz(r >> 31, r == 0);
		} else if ((rm & 0x1F) == 0) {
			carry = operand >> 31;
			r = operand;
			cpu->set_nzc(r >> 31, r == 0, carry);
		} else {
			carry = operand & (1 << ((rm & 0x1F) - 1));
			r = cpu->gpr[rd] = std::rotr(operand, rm & 0x1F);
			cpu->set_nzc(r >> 31, r == 0, carry);
		}
		break;
	case TST:
		r = operand & rm;
		cpu->set_nz(r >> 31, r == 0);
		break;
	case NEG:
		r = cpu->gpr[rd] = -rm;
		SUB_FLAGS_(0, rm);
		cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
		break;
	case CMP:
		r = operand - rm;
		SUB_FLAGS_(operand, rm);
		cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
		break;
	case CMN:
		r = operand + rm;
		ADD_FLAGS_(operand, rm);
		cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
		break;
	case ORR:
		r = cpu->gpr[rd] = operand | rm;
		cpu->set_nz(r >> 31, r == 0);
		break;
	case MUL:
		r = cpu->gpr[rd] = operand * rm;
		cpu->set_nz(r >> 31, r == 0);
		break;
	case BIC:
		r = cpu->gpr[rd] = operand & ~rm;
		cpu->set_nz(r >> 31, r == 0);
		break;
	case MVN:
		r = cpu->gpr[rd] = ~rm;
		cpu->set_nz(r >> 31, r == 0);
	}
}

template <int R, int RD>
void
thumb_alu6(Arm *cpu)
{
	if (R == 0) {
		cpu->gpr[RD] = (cpu->pc() & ~3) + ((cpu->opcode & 0xFF) << 2);
	} else {
		cpu->gpr[RD] = cpu->gpr[13] + ((cpu->opcode & 0xFF) << 2);
	}
}

template <int OP>
void
thumb_alu7(Arm *cpu)
{
	if (OP == 0) {
		cpu->gpr[13] += (cpu->opcode & 0x7F) << 2;
	} else {
		cpu->gpr[13] -= (cpu->opcode & 0x7F) << 2;
	}
}

template <int OP>
void
thumb_alu8(Arm *cpu)
{
	bool H1 = cpu->opcode >> 7 & 1;
	u32 rm = cpu->gpr[cpu->opcode >> 3 & 0xF];
	u32 rd = H1 << 3 | (cpu->opcode & 0x7);

	if (OP == 0) {
		u32 r = cpu->gpr[rd] + rm;
		if (rd == 15) {
			cpu->thumb_jump(r & ~1);
		} else {
			cpu->gpr[rd] = r;
		}
	} else if (OP == 1) {
		u32 rn = cpu->gpr[rd];
		u32 r = rn - rm;
		bool carry;
		bool overflow;
		SUB_FLAGS_(rn, rm);
		cpu->set_nzcv(r >> 31, r == 0, carry, overflow);
	} else {
		if (rd == 15) {
			cpu->thumb_jump(rm & ~1);
		} else {
			cpu->gpr[rd] = rm;
		}
	}
}

} // namespace twice

#endif
