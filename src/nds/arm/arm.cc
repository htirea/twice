#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

Arm::Arm(NDS *nds, int cpuid)
	: nds(nds),
	  cpuid(cpuid),
	  target_cycles(nds->arm_target_cycles[cpuid]),
	  cycles(nds->arm_cycles[cpuid])
{
}

void
Arm::run()
{
	if (halted) {
		cycles = target_cycles;
		return;
	}

	while (cycles < target_cycles) {
		step();
	}
}

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

} // namespace twice
