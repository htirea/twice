#ifndef LIBTWICE_ARM_H
#define LIBTWICE_ARM_H

#include "libtwice/types.h"

namespace twice {

enum CpuMode {
	MODE_SYS,
	MODE_FIQ,
	MODE_SVC,
	MODE_ABT,
	MODE_IRQ,
	MODE_UND,
	MODE_USR = 8
};

enum CpuModeBits {
	USR_MODE_BITS = 0x10,
	FIQ_MODE_BITS = 0x11,
	IRQ_MODE_BITS = 0x12,
	SVC_MODE_BITS = 0x13,
	ABT_MODE_BITS = 0x17,
	UND_MODE_BITS = 0x1B,
	SYS_MODE_BITS = 0x1F,
};

struct NDS;

struct Arm {
	Arm(NDS *nds);

	u32 gpr[16]{};
	u32 bankedr[6][3]{};
	u32 fiqr[5]{};

	NDS *nds{};

	virtual void jump(u32 addr) = 0;
};

struct Arm9 final : Arm {
	Arm9(NDS *nds);

	void jump(u32 addr);
	void cp15_write(u32 reg, u32 value);
};

struct Arm7 final : Arm {
	Arm7(NDS *nds);

	void jump(u32 addr);
};

} // namespace twice

#endif
