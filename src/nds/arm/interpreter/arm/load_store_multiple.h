#ifndef TWICE_ARM_LOAD_STORE_MULTIPLE_H
#define TWICE_ARM_LOAD_STORE_MULTIPLE_H

#include "nds/arm/interpreter/util.h"

namespace twice::arm::interpreter {

template <int P, int U, int S, int W, int L>
void
arm_block_dt(arm_cpu *cpu)
{
	u32 rn = cpu->opcode >> 16 & 0xF;
	u16 register_list = cpu->opcode & 0xFFFF;
	int count = std::popcount(register_list);
	u32 offset = 4 * count;

	u32 addr;
	u32 writeback_value;

	if (register_list == 0) {
		offset = 0x40;
	}

	if (P == 0 && U == 1) {
		addr = cpu->gpr[rn];
		writeback_value = cpu->gpr[rn] + offset;
	} else if (P == 1 && U == 1) {
		addr = cpu->gpr[rn] + 4;
		writeback_value = cpu->gpr[rn] + offset;
	} else if (P == 0 && U == 0) {
		addr = cpu->gpr[rn] - offset + 4;
		writeback_value = cpu->gpr[rn] - offset;
	} else if (P == 1 && U == 0) {
		addr = cpu->gpr[rn] - offset;
		writeback_value = cpu->gpr[rn] - offset;
	}

	addr &= ~3;

	bool writeback = W && rn != 15;
	if (W && rn == 15) {
		LOG("writeback with pc as base\n");
	}

	if (L == 1) {
		bool cpsr_written = false;

		if (S == 0) {
			cpu->ldm(addr, register_list, count);
		} else if (!(register_list >> 15)) {
			cpu->ldm_user(addr, register_list, count);
		} else {
			cpu->ldm_cpsr(addr, register_list, count);
			cpsr_written = true;
		}

		if (writeback) {
			/* If rn is included in register_list:
			 * ARMv4: no writeback
			 * ARMv5: writeback if rn is only register,
			 *        or not the last register
			 */
			if (register_list & BIT(rn)) {
				bool only = register_list == BIT(rn);
				bool last = register_list >> rn == 1;
				if (is_arm9(cpu) && (only || !last)) {
					cpu->gpr[rn] = writeback_value;
				}
			} else {
				cpu->gpr[rn] = writeback_value;
			}
		}

		/* mode switch done after writeback */
		if (cpsr_written) {
			arm_on_cpsr_write(cpu);
		}
	} else {
		if (writeback) {
			/* If rn is included in register_list:
			 * ARMv4: if rn is first store old base
			 *        else store new base
			 * ARMv5: store old base
			 */
			bool in_rlist = register_list & BIT(rn);
			bool not_first = register_list & MASK(rn);
			if (in_rlist && is_arm7(cpu) && not_first) {
				cpu->gpr[rn] = writeback_value;
			}
		}

		if (S == 0) {
			cpu->stm(addr, register_list, count);
		} else {
			cpu->stm_user(addr, register_list, count);
		}

		if (writeback) {
			cpu->gpr[rn] = writeback_value;
		}
	}
}

} // namespace twice::arm::interpreter

#endif
