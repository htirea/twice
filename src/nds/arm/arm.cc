#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

const u16 arm_cond_table[16] = {
	0xF0F0, /* EQ */
	0x0F0F, /* NE */
	0xCCCC, /* CS/HS */
	0x3333, /* CC/LO */
	0xFF00, /* MI */
	0x00FF, /* PL */
	0xAAAA, /* VS */
	0x5555, /* VC */
	0x0C0C, /* HI */
	0xF3F3, /* LS */
	0xAA55, /* GE */
	0x55AA, /* LT */
	0x0A05, /* GT */
	0xF5FA, /* LE */
	0xFFFF, /* AL */
	0x0000, /* NV -- if this is changed check the decoding for blx(1) */
};

arm_cpu::arm_cpu(nds_ctx *nds, int cpuid)
	: nds(nds),
	  cpuid(cpuid),
	  target_cycles(nds->arm_target_cycles[cpuid]),
	  cycles(nds->arm_cycles[cpuid])
{
}

arm_cpu::~arm_cpu() = default;

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

void
run_cpu(arm_cpu *cpu)
{
	if (cpu->stopped || cpu->halted) {
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
