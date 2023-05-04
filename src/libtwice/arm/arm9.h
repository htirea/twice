#ifndef LIBTWICE_ARM9_H
#define LIBTWICE_ARM9_H

#include "libtwice/arm/arm.h"

namespace twice {

enum TcmSizes {
	ITCM_SIZE = 32_KiB,
	ITCM_MASK = 32_KiB - 1,
	DTCM_SIZE = 16_KiB,
	DTCM_MASK = 16_KiB - 1,
};

struct Arm9 final : Arm {
	Arm9(NDS *nds)
		: Arm(nds)
	{
	}

	u8 itcm[ITCM_SIZE]{};
	u8 dtcm[DTCM_SIZE]{};

	u32 itcm_virtual_size{};
	u32 itcm_mask{};
	u32 dtcm_base{};
	u32 dtcm_virtual_size{};
	u32 dtcm_mask{};

	bool read_itcm{};
	bool write_itcm{};
	bool read_dtcm{};
	bool write_dtcm{};

	void jump(u32 addr) override;
	u32 fetch32(u32 addr) override;
	u16 fetch16(u32 addr) override;
	u32 load32(u32 addr) override;
	u16 load16(u32 addr) override;
	u8 load8(u32 addr) override;
	void store32(u32 addr, u32 value) override;
	void store16(u32 addr, u16 value) override;
	void store8(u32 addr, u8 value) override;

	void cp15_write(u32 reg, u32 value);

	template <typename T> T fetch(u32 addr);
	template <typename T> T load(u32 addr);
	template <typename T> void store(u32 addr, T value);
};

} // namespace twice

#endif
