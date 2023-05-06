#ifndef TWICE_ARM_H
#define TWICE_ARM_H

#include "common/types.h"

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
	Arm(NDS *nds)
		: nds(nds)
	{
	}

	u32 gpr[16]{};
	u32 bankedr[6][3]{};
	u32 fiqr[5]{};
	u32 cpsr = (1 << 7) | (1 << 6) | SYS_MODE_BITS;
	u32 pipeline[2]{};
	u32 exception_base{};

	struct Opcode {
		u32 word{};
	} opcode;

	NDS *nds{};

	u32& pc() { return gpr[15]; }

	bool in_thumb() { return cpsr & (1 << 5); }

	virtual void jump(u32 addr) = 0;
	virtual u32 fetch32(u32 addr) = 0;
	virtual u16 fetch16(u32 addr) = 0;
	virtual u32 load32(u32 addr) = 0;
	virtual u16 load16(u32 addr) = 0;
	virtual u8 load8(u32 addr) = 0;
	virtual void store32(u32 addr, u32 value) = 0;
	virtual void store16(u32 addr, u16 value) = 0;
	virtual void store8(u32 addr, u8 value) = 0;
};

} // namespace twice

#endif
