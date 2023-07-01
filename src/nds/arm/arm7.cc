#include "nds/arm/arm7.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

arm7_cpu::arm7_cpu(nds_ctx *nds)
	: arm_cpu(nds, 1)
{
}

static bool
check_cond(u32 cond, u32 cpsr)
{
	return arm_cond_table[cond] & (1 << (cpsr >> 28));
}

void
arm7_cpu::step()
{
	if (cpsr & 0x20) {
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

		u32 cond = opcode >> 28;
		if (cond == 0xE || check_cond(cond, cpsr)) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		}
	}

	if (interrupt) {
		arm_do_irq(this);
	}

	cycles += 1;
}

void
arm7_cpu::arm_jump(u32 addr)
{
	pc() = addr + 4;
	pipeline[0] = fetch32(addr);
	pipeline[1] = fetch32(addr + 4);
}

void
arm7_cpu::thumb_jump(u32 addr)
{
	pc() = addr + 2;
	pipeline[0] = fetch16(addr);
	pipeline[1] = fetch16(addr + 2);
}

void
arm7_cpu::jump_cpsr(u32 addr)
{
	cpsr = spsr();
	arm_on_cpsr_write(this);

	if (cpsr & 0x20) {
		thumb_jump(addr & ~1);
	} else {
		arm_jump(addr & ~3);
	}
}

u32
arm7_cpu::fetch32(u32 addr)
{
	return bus7_read<u32>(nds, addr);
}

u16
arm7_cpu::fetch16(u32 addr)
{
	return bus7_read<u16>(nds, addr);
}

u32
arm7_cpu::load32(u32 addr)
{
	return bus7_read<u32>(nds, addr);
}

u16
arm7_cpu::load16(u32 addr)
{
	return bus7_read<u16>(nds, addr);
}

u8
arm7_cpu::load8(u32 addr)
{
	return bus7_read<u8>(nds, addr);
}

void
arm7_cpu::store32(u32 addr, u32 value)
{
	bus7_write<u32>(nds, addr, value);
}

void
arm7_cpu::store16(u32 addr, u16 value)
{
	bus7_write<u16>(nds, addr, value);
}

void
arm7_cpu::store8(u32 addr, u8 value)
{
	bus7_write<u8>(nds, addr, value);
}

u16
arm7_cpu::ldrh(u32 addr)
{
	return std::rotr(load16(addr & ~1), (addr & 1) << 3);
}

s16
arm7_cpu::ldrsh(u32 addr)
{
	if (addr & 1) {
		return (s8)load8(addr);
	} else {
		return load16(addr);
	}
}

} // namespace twice
