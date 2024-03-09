#include "nds/arm/arm7.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/nds.h"

#include "libtwice/exception.h"

namespace twice {

void
arm7_cpu::run()
{
	if (halted) {
		*cycles = *target_cycles;
		return;
	}

	if (interrupt) {
		arm_do_irq(this);
	}

	while (*cycles < *target_cycles) {
		step();
	}
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
		if (cond == 0xE ||
				arm_cond_table[cond] & (1 << (cpsr >> 28))) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		}
	}

	if (interrupt) {
		arm_do_irq(this);
	}

	*cycles += 1;
	cycles_executed += 1;
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

template <typename T>
static T
fetch(arm7_cpu *cpu, u32 addr)
{
	u8 *p = cpu->read_pt[addr >> BUS7_PAGE_SHIFT];
	if (p) {
		return readarr<T>(p, addr & BUS7_PAGE_MASK);
	}

	return bus7_read_slow<T>(cpu->nds, addr);
}

template <typename T>
static T
load(arm7_cpu *cpu, u32 addr)
{
	u8 *p = cpu->read_pt[addr >> BUS7_PAGE_SHIFT];
	if (p) {
		return readarr<T>(p, addr & BUS7_PAGE_MASK);
	}

	return bus7_read_slow<T>(cpu->nds, addr);
}

template <typename T>
static void
store(arm7_cpu *cpu, u32 addr, T value)
{
	u8 *p = cpu->write_pt[addr >> BUS7_PAGE_SHIFT];
	if (p) {
		writearr<T>(p, addr & BUS7_PAGE_MASK, value);
		return;
	}

	bus7_write_slow<T>(cpu->nds, addr, value);
}

u32
arm7_cpu::fetch32(u32 addr)
{
	return fetch<u32>(this, addr);
}

u16
arm7_cpu::fetch16(u32 addr)
{
	return fetch<u16>(this, addr);
}

u32
arm7_cpu::load32(u32 addr)
{
	return load<u32>(this, addr);
}

u16
arm7_cpu::load16(u32 addr)
{
	return load<u16>(this, addr);
}

u8
arm7_cpu::load8(u32 addr)
{
	return load<u8>(this, addr);
}

void
arm7_cpu::store32(u32 addr, u32 value)
{
	store<u32>(this, addr, value);
}

void
arm7_cpu::store16(u32 addr, u16 value)
{
	store<u16>(this, addr, value);
}

void
arm7_cpu::store8(u32 addr, u8 value)
{
	store<u8>(this, addr, value);
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

void
arm7_direct_boot(arm7_cpu *cpu, u32 entry_addr)
{
	cpu->gpr[12] = entry_addr;
	cpu->gpr[13] = 0x0380FD80;
	cpu->gpr[14] = entry_addr;
	cpu->bankedr[arm_cpu::MODE_IRQ][0] = 0x0380FF80;
	cpu->bankedr[arm_cpu::MODE_SVC][0] = 0x0380FFC0;
	cpu->arm_jump(entry_addr);
}

void
arm7_init_page_tables(arm7_cpu *cpu)
{
	cpu->read_pt = cpu->nds->bus7_read_pt;
	cpu->write_pt = cpu->nds->bus7_write_pt;
}

} // namespace twice
