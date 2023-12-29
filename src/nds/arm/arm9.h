#ifndef TWICE_ARM9_H
#define TWICE_ARM9_H

#include "nds/arm/arm.h"

namespace twice {

struct arm9_cpu final : arm_cpu {
	u8 *fetch_pt[PAGE_TABLE_SIZE]{};
	u8 *load_pt[PAGE_TABLE_SIZE]{};
	u8 *store_pt[PAGE_TABLE_SIZE]{};

	enum {
		ITCM_SIZE = 32_KiB,
		ITCM_MASK = 32_KiB - 1,
		DTCM_SIZE = 16_KiB,
		DTCM_MASK = 16_KiB - 1,
	};

	u8 itcm[ITCM_SIZE]{};
	u8 dtcm[DTCM_SIZE]{};

	u32 itcm_addr_mask{};
	u32 itcm_array_mask{};
	u32 dtcm_base{};
	u32 dtcm_addr_mask{};
	u32 dtcm_array_mask{};

	bool read_itcm{};
	bool write_itcm{};
	bool read_dtcm{};
	bool write_dtcm{};

	u32 ctrl_reg{ 0x78 };
	u32 dtcm_reg{};
	u32 itcm_reg{};

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
		if ((IE & IF) && (IME & 1)) {
			halted &= ~CPU_HALT;
		}

		return halted;
	}
};

void arm9_direct_boot(arm9_cpu *gpu, u32 entry_addr);
void update_arm9_page_tables(arm9_cpu *cpu);
u32 cp15_read(arm9_cpu *cpu, u32 reg);
void cp15_write(arm9_cpu *cpu, u32 reg, u32 value);

} // namespace twice

#endif
