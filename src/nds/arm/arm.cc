#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

static u32
mode_bits_to_mode(u32 bits)
{
	switch (bits) {
	case SYS_MODE_BITS:
		return MODE_SYS;
	case FIQ_MODE_BITS:
		return MODE_FIQ;
	case SVC_MODE_BITS:
		return MODE_SVC;
	case ABT_MODE_BITS:
		return MODE_ABT;
	case IRQ_MODE_BITS:
		return MODE_IRQ;
	case UND_MODE_BITS:
		return MODE_UND;
	case USR_MODE_BITS:
		return MODE_USR;
	}

	throw TwiceException("invalid mode bits");
}

void
Arm::swap_registers(u32 old_mode, u32 new_mode)
{
	static_assert((MODE_SYS & 7) == (MODE_USR & 7));
	old_mode &= 7;
	new_mode &= 7;

	if (new_mode == old_mode) {
		return;
	}

	bankedr[old_mode][0] = gpr[13];
	bankedr[old_mode][1] = gpr[14];
	bankedr[old_mode][2] = bankedr[0][2];

	gpr[13] = bankedr[new_mode][0];
	gpr[14] = bankedr[new_mode][1];
	bankedr[0][2] = bankedr[new_mode][2];

	if (new_mode == MODE_FIQ || old_mode == MODE_FIQ) {
		for (int i = 0; i < 5; i++) {
			std::swap(gpr[8 + i], fiqr[i]);
		}
	}
}

void
Arm::switch_mode(u32 new_mode)
{
	if (new_mode != mode) {
		swap_registers(mode, new_mode);
	}

	mode = new_mode;
}

void
Arm::on_cpsr_write()
{
	u32 new_mode = mode_bits_to_mode(cpsr & 0x1F);

	switch_mode(new_mode);

	check_interrupt();
}

static bool
check_cond(u32 cpsr, u32 cond)
{
	if (cond == 0xE) {
		return true;
	}

	bool N = cpsr & (1 << 31);
	bool Z = cpsr & (1 << 30);
	bool C = cpsr & (1 << 29);
	bool V = cpsr & (1 << 28);

	switch (cond) {
	case 0x0:
		return Z;
	case 0x1:
		return !Z;
	case 0x2:
		return C;
	case 0x3:
		return !C;
	case 0x4:
		return N;
	case 0x5:
		return !N;
	case 0x6:
		return V;
	case 0x7:
		return !V;
	case 0x8:
		return C && !Z;
	case 0x9:
		return !C || Z;
	case 0xA:
		return N == V;
	case 0xB:
		return N != V;
	case 0xC:
		return !Z && N == V;
	case 0xD:
		return Z || N != V;
	}

	fprintf(stderr, "unknown cond\n");
	return false;
}

void
Arm::do_irq()
{
	u32 old_cpsr = cpsr;

	cpsr &= ~0xBF;
	cpsr |= 0x92;
	switch_mode(MODE_IRQ);
	interrupt = false;

	gpr[14] = pc() - (in_thumb() ? 2 : 4) + 4;
	spsr() = old_cpsr;
	arm_jump(exception_base + 0x18);
}

void
Arm9::step()
{
	if (in_thumb()) {
		pc() += 2;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch16(pc());
		thumb_inst_lut[opcode >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch32(pc());

		if (check_cond(cpsr, opcode >> 28)) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		} else if ((opcode & 0xFE000000) == 0xFA000000) {
			bool H = opcode & (1 << 24);
			s32 offset = ((s32)(opcode << 8) >> 6) + (H << 1);

			gpr[14] = pc() - 4;
			set_t(1);
			thumb_jump(pc() + offset);
		}
	}

	if (interrupt) {
		do_irq();
	}

	nds->cycles += 1;
}

void
Arm7::step()
{
	if (in_thumb()) {
		pc() += 2;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch16(pc());
		thumb_inst_lut[opcode >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch32(pc());

		if (check_cond(cpsr, opcode >> 28)) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		}
	}

	if (interrupt) {
		do_irq();
	}
}

} // namespace twice
