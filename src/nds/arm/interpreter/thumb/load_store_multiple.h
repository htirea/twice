#ifndef TWICE_THUMB_LOAD_STORE_MULTIPLE_H
#define TWICE_THUMB_LOAD_STORE_MULTIPLE_H

#include "nds/arm/interpreter/util.h"

namespace twice {

template <int L, int RN>
void
thumb_ldm_stm(Arm *cpu)
{
	u8 register_list = cpu->opcode & 0xFF;
	u32 offset = 4 * std::popcount(register_list);

	u32 addr;
	u32 writeback_value;

	if (register_list == 0) {
		offset = 0x40;
	}

	addr = cpu->gpr[RN];
	writeback_value = cpu->gpr[RN] + offset;

	addr &= ~3;

	if (L == 1) {
		TWICE_ARM_LDM_(0, 7, cpu->gpr, 0);

		if (cpu->is_arm7() && register_list == 0) {
			cpu->thumb_jump(cpu->load32(addr) & ~1);
		}

		/* If rn is included in register list:
		 * ARMv4 / ARMv5: no writeback
		 */
		if (!(register_list & (1 << RN))) {
			cpu->gpr[RN] = writeback_value;
		}
	} else {
		/* If rn is included in register list:
		 * ARMv4: If rn is first store old base
		 *        else store new base
		 * ARMv5: store old base
		 * (same as arm stm)
		 */
		bool in_rlist = register_list & (1 << RN);
		bool not_first = register_list & ((1 << RN) - 1);
		if (in_rlist && cpu->is_arm7() && not_first) {
			cpu->gpr[RN] = writeback_value;
		}

		TWICE_ARM_STM_(0, 7, cpu->gpr, 0);

		if (cpu->is_arm7() && register_list == 0) {
			cpu->store32(addr, cpu->pc() + 2);
		}

		cpu->gpr[RN] = writeback_value;
	}
}

template <int L, int R>
void
thumb_push_pop(Arm *cpu)
{
	u8 register_list = cpu->opcode & 0xFF;
	u32 offset = 4 * (R + std::popcount(register_list));

	if (L == 1) {
		u32 addr = cpu->gpr[13] & ~3;
		TWICE_ARM_LDM_(0, 7, cpu->gpr, 0);

		if (R == 1) {
			u32 value = cpu->load32(addr);
			if (cpu->is_arm9()) {
				thumb_do_bx(cpu, value);
			} else {
				cpu->thumb_jump(value & ~1);
			}
		}

		cpu->gpr[13] += offset;
	} else {
		u32 addr = (cpu->gpr[13] & ~3) - offset;
		TWICE_ARM_STM_(0, 7, cpu->gpr, 0);

		if (R == 1) {
			cpu->store32(addr, cpu->gpr[14]);
		}

		cpu->gpr[13] -= offset;
	}
}

} // namespace twice

#endif
