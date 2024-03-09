#ifndef TWICE_ARM7_H
#define TWICE_ARM7_H

#include "nds/arm/arm.h"

namespace twice {

struct arm7_cpu final : arm_cpu {
	u8 **read_pt{};
	u8 **write_pt{};

	void run() override;
	void step() override;
	void arm_jump(u32 addr) override;
	void thumb_jump(u32 addr) override;
	void jump_cpsr(u32 addr) override;
	u32 fetch32(u32 addr) override;
	u16 fetch16(u32 addr) override;
	u32 load32(u32 addr) override;
	u16 load16(u32 addr) override;
	u8 load8(u32 addr) override;
	void store32(u32 addr, u32 value) override;
	void store16(u32 addr, u16 value) override;
	void store8(u32 addr, u8 value) override;
	u16 ldrh(u32 addr) override;
	s16 ldrsh(u32 addr) override;

	bool check_halted() override
	{
		if (IE & IF) {
			halted &= ~CPU_HALT;
		}

		return halted;
	}
};

void arm7_direct_boot(arm7_cpu *cpu, u32 entry_addr);
void arm7_init_page_tables(arm7_cpu *cpu);

} // namespace twice

#endif
