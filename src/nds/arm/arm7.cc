#include "nds/arm/arm7.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

Arm7::Arm7(NDS *nds)
	: Arm(nds, 1)
{
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

		if (check_cond(opcode >> 28)) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		}
	}

	if (interrupt) {
		do_irq();
	}

	cycles += 1;
}

void
Arm7::jump(u32 addr)
{
	if (in_thumb()) {
		thumb_jump(addr);
	} else {
		arm_jump(addr);
	}
}

void
Arm7::arm_jump(u32 addr)
{
	pc() = addr + 4;
	pipeline[0] = fetch32(addr);
	pipeline[1] = fetch32(addr + 4);
}

void
Arm7::thumb_jump(u32 addr)
{
	pc() = addr + 2;
	pipeline[0] = fetch16(addr);
	pipeline[1] = fetch16(addr + 2);
}

void
Arm7::jump_cpsr(u32 addr)
{
	cpsr = spsr();
	on_cpsr_write();

	if (in_thumb()) {
		thumb_jump(addr & ~1);
	} else {
		arm_jump(addr & ~3);
	}
}

u32
Arm7::fetch32(u32 addr)
{
	return bus7_read<u32>(nds, addr);
}

u16
Arm7::fetch16(u32 addr)
{
	return bus7_read<u16>(nds, addr);
}

u32
Arm7::load32(u32 addr)
{
	return bus7_read<u32>(nds, addr);
}

u16
Arm7::load16(u32 addr)
{
	return bus7_read<u16>(nds, addr);
}

u8
Arm7::load8(u32 addr)
{
	return bus7_read<u8>(nds, addr);
}

void
Arm7::store32(u32 addr, u32 value)
{
	bus7_write<u32>(nds, addr, value);
}

void
Arm7::store16(u32 addr, u16 value)
{
	bus7_write<u16>(nds, addr, value);
}

void
Arm7::store8(u32 addr, u8 value)
{
	bus7_write<u8>(nds, addr, value);
}

u16
Arm7::ldrh(u32 addr)
{
	return std::rotr(load16(addr & ~1), (addr & 1) << 3);
}

s16
Arm7::ldrsh(u32 addr)
{
	if (addr & 1) {
		return (s8)load8(addr);
	} else {
		return load16(addr);
	}
}

} // namespace twice
