#ifndef TWICE_ARM_STATUS_REG_H
#define TWICE_ARM_STATUS_REG_H

#include "nds/arm/interpreter/util.h"

namespace twice {

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

} // namespace twice

#endif
