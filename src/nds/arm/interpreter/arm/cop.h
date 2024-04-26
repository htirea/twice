#ifndef TWICE_ARM_COP_H
#define TWICE_ARM_COP_H

#include "nds/arm/arm9.h"
#include "nds/arm/interpreter/arm/exception.h"
#include "nds/arm/interpreter/util.h"

namespace twice::arm::interpreter {

template <typename CPUT, int OP1, int L, int OP2>
void
arm_cop_reg(CPUT *cpu)
{
	u32 cn = cpu->opcode >> 16 & 0xF;
	u32 rd = cpu->opcode >> 12 & 0xF;
	u32 cp_num = cpu->opcode >> 8 & 0xF;
	u32 cm = cpu->opcode & 0xF;

	if (L == 0) {
		if (is_arm7(cpu)) {
			LOG("arm7 mcr\n");
			return arm_noop(cpu);
		}

		if (cp_num != 15) {
			throw twice_error("arm9 mcr cp_num != 15");
		}

		if (OP1 != 0) {
			throw twice_error("arm9 mcr op1 != 0");
		}

		u32 reg = cn << 8 | cm << 4 | OP2;
		u32 value = cpu->gpr[rd];
		if (rd == 15) {
			value += 4;
		}
		cp15_write((arm9_cpu *)cpu, reg, value);

		/* TODO: MCR timings? */
		cpu->add_code_cycles(1 + 1);
	} else {
		u32 value;

		if (is_arm7(cpu)) {
			if (cp_num != 14) {
				LOG("arm7 mrc with cp_num %u\n", cp_num);
				return arm_undefined(cpu);
			}

			value = cpu->pipeline[1];
		} else {
			if (cp_num != 15) {
				LOG("arm9 mrc with cp_num %u\n", cp_num);
				return arm_undefined(cpu);
			}

			if (OP1 != 0) {
				throw twice_error("arm9 mrc op1 != 0");
			}

			if (!in_privileged_mode(cpu)) {
				throw twice_error(
						"arm9 mrc not in privileged mode");
			}

			u32 reg = cn << 8 | cm << 4 | OP2;
			value = cp15_read((arm9_cpu *)cpu, reg);
		}

		if (rd == 15) {
			u32 mask = 0xF0000000;
			cpu->cpsr = (cpu->cpsr & ~mask) | (value & mask);
		} else {
			cpu->gpr[rd] = value;
		}

		/* TODO: MRC cycles ? */
		cpu->add_code_cycles(3);
	}
}

template <typename CPUT>
void
arm_cdp(CPUT *)
{
	throw twice_error("arm cdp");
}

template <typename CPUT>
void
arm_ldc(CPUT *)
{
	throw twice_error("arm ldc");
}

template <typename CPUT>
void
arm_stc(CPUT *)
{
	throw twice_error("arm stc");
}

template <typename CPUT>
void
arm_mcrr(CPUT *)
{
	throw twice_error("arm mcrr");
}

template <typename CPUT>
void
arm_mrrc(CPUT *)
{
	throw twice_error("arm mrrc");
}

} // namespace twice::arm::interpreter

#endif
