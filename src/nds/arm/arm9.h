#ifndef TWICE_ARM9_H
#define TWICE_ARM9_H

#include "nds/arm/arm.h"
#include "nds/mem/bus.h"

namespace twice {

struct arm9_cpu final : arm_cpu {
	enum {
		FETCH,
		LOAD,
		STORE,
	};

	u8 *pages[3][BUS9_PAGE_TABLE_SIZE]{};
	std::array<u8, 4> timings[3][BUS9_PAGE_TABLE_SIZE]{};

	enum {
		ITCM_SIZE = 32_KiB,
		ITCM_MASK = 32_KiB - 1,
		DTCM_SIZE = 16_KiB,
		DTCM_MASK = 16_KiB - 1,
	};

	u8 itcm[ITCM_SIZE]{};
	u8 dtcm[DTCM_SIZE]{};

	u64 itcm_end{};
	u32 itcm_array_mask{};
	u32 dtcm_base{};
	u64 dtcm_end{};
	u32 dtcm_array_mask{};

	bool read_itcm{};
	bool write_itcm{};
	bool read_dtcm{};
	bool write_dtcm{};

	u32 ctrl_reg{ 0x78 };
	u32 dtcm_reg{};
	u32 itcm_reg{};

	void add_ldr_cycles() override
	{
		/* TODO: handle properly */
		u32 x = (std::max(code_cycles, data_cycles) + 1) & ~1;
		*cycles += x;
		cycles_executed += x;
	}

	void add_str_cycles(u32 extra = 0) override
	{
		/* TODO: handle properly */

		u32 x = ((std::max(code_cycles, data_cycles) + 1) & ~1) +
		        extra;
		*cycles += x;
		cycles_executed += x;
	}

	void run() override;
	void step() override;
	void arm_jump(u32 addr) override;
	void thumb_jump(u32 addr) override;
	void jump_cpsr(u32 addr) override;
	u8 fetch32n(u32 addr, u32 *result);
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
		if ((IE & IF) && (IME & 1)) {
			halted &= ~CPU_HALT;
		}

		return halted;
	}
};

void arm9_direct_boot(arm9_cpu *gpu, u32 entry_addr);
u32 cp15_read(arm9_cpu *cpu, u32 reg);
void cp15_write(arm9_cpu *cpu, u32 reg, u32 value);
void update_arm9_page_tables(arm9_cpu *cpu, u64 start, u64 end);

} // namespace twice

#endif
