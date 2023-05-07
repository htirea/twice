#ifndef TWICE_ARM9_H
#define TWICE_ARM9_H

#include "nds/arm/arm.h"

namespace twice {

enum TcmSizes {
	ITCM_SIZE = 32_KiB,
	ITCM_MASK = 32_KiB - 1,
	DTCM_SIZE = 16_KiB,
	DTCM_MASK = 16_KiB - 1,
};

struct Arm9 final : Arm {
	Arm9(NDS *nds);

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

	void jump(u32 addr) override;
	void arm_jump(u32 addr) override;
	void thumb_jump(u32 addr) override;
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

	void step();
	void cp15_write(u32 reg, u32 value);

	template <typename T> T fetch(u32 addr);
	template <typename T> T load(u32 addr);
	template <typename T> void store(u32 addr, T value);
};

} // namespace twice

#endif
