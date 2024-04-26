#ifndef TWICE_THUMB_LOAD_STORE_MULTIPLE_H
#define TWICE_THUMB_LOAD_STORE_MULTIPLE_H

#include "nds/arm/interpreter/util.h"

namespace twice::arm::interpreter {

template <typename CPUT, int L, int RN>
void
thumb_ldm_stm(CPUT *cpu)
{
	u8 register_list = cpu->opcode & 0xFF;
	int count = std::popcount(register_list);
	u32 offset = 4 * count;

	u32 addr;
	u32 writeback_value;

	if (register_list == 0) {
		offset = 0x40;
	}

	addr = cpu->gpr[RN];
	writeback_value = cpu->gpr[RN] + offset;

	addr &= ~3;

	if (L == 1) {
		cpu->thumb_ldm(addr, register_list, count);

		/* If rn is included in register list:
		 * ARMv4 / ARMv5: no writeback
		 */
		if (!(register_list & BIT(RN))) {
			cpu->gpr[RN] = writeback_value;
		}
	} else {
		/* If rn is included in register list:
		 * ARMv4: If rn is first store old base
		 *        else store new base
		 * ARMv5: store old base
		 * (same as arm stm)
		 */
		bool in_rlist = register_list & BIT(RN);
		bool not_first = register_list & MASK(RN);
		if (in_rlist && is_arm7(cpu) && not_first) {
			cpu->gpr[RN] = writeback_value;
		}

		cpu->thumb_stm(addr, register_list, count);
		cpu->gpr[RN] = writeback_value;
	}
}

template <typename CPUT, int L, int R>
void
thumb_push_pop(CPUT *cpu)
{
	u16 register_list = cpu->opcode & 0xFF;
	int count = R + std::popcount(register_list);
	u32 offset = 4 * count;
	u32 values[9]{};
	u32 *p = values;

	if (L == 1) {
		u32 addr = cpu->gpr[13] & ~3;
		cpu->gpr[13] += offset;

		cpu->load_multiple(addr, count, values);

		p = arm::interpreter::ldm_loop(register_list, 8, p, cpu->gpr);

		cpu->add_ldr_cycles();

		if (R == 1) {
			if (is_arm9(cpu)) {
				thumb_do_bx(cpu, *p);
			} else {
				cpu->thumb_jump(*p & ~1);
			}
		}

	} else {
		u32 addr = (cpu->gpr[13] & ~3) - offset;
		cpu->gpr[13] -= offset;

		p = stm_loop(register_list, 8, cpu->gpr, p);
		if (R == 1) {
			*p++ = cpu->gpr[14];
		}

		cpu->store_multiple(addr, count, values);
		cpu->add_str_cycles();
	}
}

} // namespace twice::arm::interpreter

#endif
