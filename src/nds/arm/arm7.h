#ifndef TWICE_ARM7_H
#define TWICE_ARM7_H

#include "nds/arm/arm.h"

namespace twice {

struct arm7_cpu final : arm_cpu {
	u8 **read_pt{};
	u8 **write_pt{};
	std::array<u8, 4> *code_tt{};
	std::array<u8, 4> *data_tt{};

	void add_ldr_cycles() override
	{
		u32 x = code_cycles + data_cycles + 1;
		*cycles += x;
		*cycles += x;
	}

	void add_str_cycles(u32 extra = 0) override
	{
		/* TODO: should be 2N */
		u32 x = code_cycles + data_cycles + extra;
		*cycles += x;
		cycles_executed += x;
	}

	void run() override;
	void step() override;
	void arm_jump(u32 addr) override;
	void thumb_jump(u32 addr) override;
	void jump_cpsr(u32 addr) override;
	u8 fetch32n(u32 addr, u32 *result);
	u8 fetch32s(u32 addr, u32 *result);
	u8 fetch16n(u32 addr, u32 *result);
	u8 fetch16s(u32 addr, u32 *result);
	u32 load32n(u32 addr) override;
	u32 load32s(u32 addr) override;
	u16 load16n(u32 addr) override;
	u8 load8n(u32 addr) override;
	void store32n(u32 addr, u32 value) override;
	void store32s(u32 addr, u32 value) override;
	void store16n(u32 addr, u16 value) override;
	void store8n(u32 addr, u8 value) override;
	void load_multiple(u32 addr, int count, u32 *values) override;
	void store_multiple(u32 addr, int count, u32 *values) override;
	u16 ldrh(u32 addr) override;
	s16 ldrsh(u32 addr) override;
	void ldm(u32 addr, u16 register_list, int count) override;
	void ldm_user(u32 addr, u16 register_list, int count) override;
	void ldm_cpsr(u32 addr, u16 register_list, int count) override;
	void stm(u32 addr, u16 register_list, int count) override;
	void stm_user(u32 addr, u16 register_list, int count) override;
	void thumb_ldm(u32 addr, u16 register_list, int count) override;
	void thumb_stm(u32 addr, u16 register_list, int count) override;

	bool check_halted() override
	{
		if (IE & IF) {
			halted &= ~CPU_HALT;
		}

		return halted;
	}
};

void arm7_direct_boot(arm7_cpu *cpu, u32 entry_addr);
void arm7_init_tables(arm7_cpu *cpu);

} // namespace twice

#endif
