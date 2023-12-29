#include "nds/arm/arm7.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

void
arm7_direct_boot(arm7_cpu *cpu, u32 entry_addr)
{
	cpu->gpr[12] = entry_addr;
	cpu->gpr[13] = 0x0380FD80;
	cpu->gpr[14] = entry_addr;
	cpu->bankedr[arm_cpu::MODE_IRQ][0] = 0x0380FF80;
	cpu->bankedr[arm_cpu::MODE_SVC][0] = 0x0380FFC0;

	update_arm7_page_tables(cpu);
	cpu->arm_jump(entry_addr);
}

void
arm7_cpu::run()
{
	if (halted) {
		if (check_halted()) {
			*cycles = *target_cycles;
			return;
		} else if (interrupt) {
			arm_do_irq(this);
		}
	}

	while (*cycles < *target_cycles) {
		step();
	}
}

static bool check_cond(u32 cond, u32 cpsr);

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

	*cycles += 1;
	cycles_executed += 1;
}

static bool
check_cond(u32 cond, u32 cpsr)
{
	return arm_cond_table[cond] & (1 << (cpsr >> 28));
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
	u8 *p = cpu->pt[addr >> arm_cpu::PAGE_SHIFT];
	if (p) {
		return readarr<T>(p, addr & arm_cpu::PAGE_MASK);
	}

	return bus7_read<T>(cpu->nds, addr);
}

template <typename T>
static T
load(arm7_cpu *cpu, u32 addr)
{
	u8 *p = cpu->pt[addr >> arm_cpu::PAGE_SHIFT];
	if (p) {
		return readarr<T>(p, addr & arm_cpu::PAGE_MASK);
	}

	return bus7_read<T>(cpu->nds, addr);
}

template <typename T>
static void
store(arm7_cpu *cpu, u32 addr, T value)
{
	u8 *p = cpu->pt[addr >> arm_cpu::PAGE_SHIFT];
	if (p) {
		writearr<T>(p, addr & arm_cpu::PAGE_MASK, value);
		return;
	}

	bus7_write<T>(cpu->nds, addr, value);
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

static void
update_arm7_page_table(nds_ctx *nds, u8 **pt, u64 start, u64 end)
{
	for (u64 addr = start; addr < end; addr += arm_cpu::PAGE_SIZE) {
		u64 page = addr >> arm_cpu::PAGE_SHIFT;
		switch (addr >> 23) {
		case 0x20 >> 3:
		case 0x28 >> 3:
			pt[page] = &nds->main_ram[addr & MAIN_RAM_MASK];
			break;
		case 0x30 >> 3:
		{
			u8 *p = nds->shared_wram_p[1];
			pt[page] = &p[addr & nds->shared_wram_mask[1]];
			break;
		}
		case 0x38 >> 3:
			pt[page] = &nds->arm7_wram[addr & ARM7_WRAM_MASK];
			break;
		default:
			pt[page] = nullptr;
		}
	}
}

void
update_arm7_page_tables(arm7_cpu *cpu)
{
	update_arm7_page_table(cpu->nds, cpu->pt, 0, 0x100000000);
}

} // namespace twice
