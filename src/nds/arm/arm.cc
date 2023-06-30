#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

arm_cpu::arm_cpu(nds_ctx *nds, int cpuid)
	: nds(nds),
	  cpuid(cpuid),
	  target_cycles(nds->arm_target_cycles[cpuid]),
	  cycles(nds->arm_cycles[cpuid])
{
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

	throw twice_error("invalid mode bits");
}

static void
swap_registers(arm_cpu *cpu, u32 old_mode, u32 new_mode)
{
	static_assert((MODE_SYS & 7) == (MODE_USR & 7));
	old_mode &= 7;
	new_mode &= 7;

	if (new_mode == old_mode) {
		return;
	}

	cpu->bankedr[old_mode][0] = cpu->gpr[13];
	cpu->bankedr[old_mode][1] = cpu->gpr[14];
	cpu->bankedr[old_mode][2] = cpu->bankedr[0][2];

	cpu->gpr[13] = cpu->bankedr[new_mode][0];
	cpu->gpr[14] = cpu->bankedr[new_mode][1];
	cpu->bankedr[0][2] = cpu->bankedr[new_mode][2];

	if (new_mode == MODE_FIQ || old_mode == MODE_FIQ) {
		for (int i = 0; i < 5; i++) {
			std::swap(cpu->gpr[8 + i], cpu->fiqr[i]);
		}
	}
}

void
arm_switch_mode(arm_cpu *cpu, u32 new_mode)
{
	if (new_mode != cpu->mode) {
		swap_registers(cpu, cpu->mode, new_mode);
	}

	cpu->mode = new_mode;
}

void
arm_check_interrupt(arm_cpu *cpu)
{
	cpu->interrupt = !(cpu->cpsr & BIT(7)) && (cpu->IME & 1) &&
			(cpu->IE & cpu->IF);
}

void
arm_on_cpsr_write(arm_cpu *cpu)
{
	u32 new_mode = mode_bits_to_mode(cpu->cpsr & 0x1F);

	arm_switch_mode(cpu, new_mode);

	arm_check_interrupt(cpu);
}

void
arm_do_irq(arm_cpu *cpu)
{
	u32 old_cpsr = cpu->cpsr;
	u32 ret_addr = cpu->pc() - (cpu->cpsr & 0x20 ? 2 : 4) + 4;

	cpu->cpsr &= ~0xBF;
	cpu->cpsr |= 0x92;
	arm_switch_mode(cpu, MODE_IRQ);
	cpu->interrupt = false;

	cpu->gpr[14] = ret_addr;
	cpu->spsr() = old_cpsr;
	cpu->arm_jump(cpu->exception_base + 0x18);
}

bool
arm_check_cond(arm_cpu *cpu, u32 cond)
{
	if (cond == 0xE) {
		return true;
	}

	bool N = cpu->cpsr & BIT(31);
	bool Z = cpu->cpsr & BIT(30);
	bool C = cpu->cpsr & BIT(29);
	bool V = cpu->cpsr & BIT(28);

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

	/* if this is changed then change blx(1) decoding as well */
	return false;
}

void
run_cpu(arm_cpu *cpu)
{
	if (cpu->halted) {
		cpu->cycles = cpu->target_cycles;
		return;
	}

	while (cpu->cycles < cpu->target_cycles) {
		cpu->step();
	}
}

void
force_stop_cpu(arm_cpu *cpu)
{
	cpu->target_cycles = cpu->cycles;
}

void
request_interrupt(arm_cpu *cpu, int bit)
{
	cpu->IF |= BIT(bit);
	arm_check_interrupt(cpu);
}

} // namespace twice
