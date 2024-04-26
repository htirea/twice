#include "nds/arm/arm7.h"
#include "nds/arm/interpreter/lut_gen_tables.h"
#include "nds/arm/interpreter/util.h"
#include "nds/nds.h"

#include "libtwice/exception.h"

namespace twice {

const auto arm7_inst_lut =
		arm::interpreter::gen::gen_array<void (*)(arm7_cpu *), 4096>(
				arm::interpreter::gen::gen_arm_lut<arm7_cpu>);
const auto thumb7_inst_lut = arm::interpreter::gen::gen_array<
		void (*)(arm7_cpu *), 1024>(
		arm::interpreter::gen::gen_thumb_lut<arm7_cpu>);

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
		code_cycles = fetch16s(pc(), &pipeline[1]);
		thumb7_inst_lut[opcode >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		code_cycles = fetch32s(pc(), &pipeline[1]);

		u32 cond = opcode >> 28;
		if (cond == 0xE ||
				arm_cond_table[cond] & (1 << (cpsr >> 28))) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm7_inst_lut[op1 << 4 | op2](this);
		} else {
			add_code_cycles();
		}
	}

	if (interrupt) {
		arm_do_irq(this);
	}
}

void
arm7_cpu::arm_jump(u32 addr)
{
	pc() = addr + 4;
	*cycles += fetch32n(addr, &pipeline[0]);
	*cycles += fetch32s(addr + 4, &pipeline[1]);
}

void
arm7_cpu::thumb_jump(u32 addr)
{
	pc() = addr + 2;
	*cycles += fetch16n(addr, &pipeline[0]);
	*cycles += fetch16s(addr + 2, &pipeline[1]);
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

template <typename T, int Idx>
static u8
fetch(arm7_cpu *cpu, u32 addr, u32 *result)
{
	u8 *p = cpu->read_pt[addr >> BUS7_PAGE_SHIFT];
	if (p) {
		*result = readarr<T>(p, addr & BUS7_PAGE_MASK);
	} else {
		*result = bus7_read_slow<T>(cpu->nds, addr);
	}

	return cpu->code_tt[addr >> BUS_TIMING_SHIFT][Idx];
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

static u32 *
load_multiple_from_page(
		arm7_cpu *cpu, u32 page, u32 addr, int count, u32 *values)
{
	u8 *p = cpu->read_pt[page];
	if (p) {
		for (; count--; addr += 4) {
			*values++ = readarr<u32>(p, addr & BUS7_PAGE_MASK);
		}
	} else {
		for (; count--; addr += 4) {
			*values++ = bus7_read_slow<u32>(cpu->nds, addr);
		}
	}

	return values;
}

static u32 *
store_multiple_to_page(
		arm7_cpu *cpu, u32 page, u32 addr, int count, u32 *values)
{
	u8 *p = cpu->write_pt[page];
	if (p) {
		for (; count--; addr += 4) {
			writearr<u32>(p, addr & BUS7_PAGE_MASK, *values++);
		}
	} else {
		for (; count--; addr += 4) {
			bus7_write_slow<u32>(cpu->nds, addr, *values++);
		}
	}

	return values;
}

template <int L>
static u32
load_store_multiple(arm7_cpu *cpu, u32 addr, int count, u32 *values)
{
	if (count == 0) {
		return 0;
	}

	u32 start_page = addr >> BUS7_PAGE_SHIFT;
	u32 end_page = (addr + 4 * (count - 1)) >> BUS7_PAGE_SHIFT;

	if (start_page == end_page) {
		if (L) {
			load_multiple_from_page(
					cpu, start_page, addr, count, values);
		} else {
			store_multiple_to_page(
					cpu, start_page, addr, count, values);
		}

		auto& timings = cpu->data_tt[addr >> BUS_TIMING_SHIFT];
		return timings[0] + (count - 1) * timings[1];
	} else {
		u32 end_addr = end_page << BUS7_PAGE_SHIFT;
		int count1 = (end_addr - addr) >> 2;

		if (L) {
			values = load_multiple_from_page(
					cpu, start_page, addr, count1, values);
			load_multiple_from_page(cpu, end_page, end_addr,
					count - count1, values);
		} else {
			values = store_multiple_to_page(
					cpu, start_page, addr, count1, values);
			store_multiple_to_page(cpu, end_page, end_addr,
					count - count1, values);
		}

		auto& timings1 = cpu->data_tt[addr >> BUS_TIMING_SHIFT];
		auto& timings2 = cpu->data_tt[end_addr >> BUS_TIMING_SHIFT];
		return timings1[0] + (count1 - 1) * timings1[1] +
		       (count - count1) * timings2[1];
	}
}

u8
arm7_cpu::fetch32n(u32 addr, u32 *result)
{
	return fetch<u32, 0>(this, addr, result);
}

u8
arm7_cpu::fetch32s(u32 addr, u32 *result)
{
	return fetch<u32, 1>(this, addr, result);
}

u8
arm7_cpu::fetch16n(u32 addr, u32 *result)
{
	return fetch<u16, 2>(this, addr, result);
}

u8
arm7_cpu::fetch16s(u32 addr, u32 *result)
{
	return fetch<u16, 3>(this, addr, result);
}

u32
arm7_cpu::load32n(u32 addr)
{
	data_cycles = data_tt[addr >> BUS_TIMING_SHIFT][0];
	return load<u32>(this, addr);
}

u32
arm7_cpu::load32s(u32 addr)
{
	data_cycles += data_tt[addr >> BUS_TIMING_SHIFT][1];
	return load<u32>(this, addr);
}

u16
arm7_cpu::load16n(u32 addr)
{
	data_cycles = data_tt[addr >> BUS_TIMING_SHIFT][2];
	return load<u16>(this, addr);
}

u8
arm7_cpu::load8n(u32 addr)
{
	data_cycles = data_tt[addr >> BUS_TIMING_SHIFT][2];
	return load<u8>(this, addr);
}

void
arm7_cpu::store32n(u32 addr, u32 value)
{
	data_cycles = data_tt[addr >> BUS_TIMING_SHIFT][0];
	store<u32>(this, addr, value);
}

void
arm7_cpu::store32s(u32 addr, u32 value)
{
	data_cycles += data_tt[addr >> BUS_TIMING_SHIFT][1];
	store<u32>(this, addr, value);
}

void
arm7_cpu::store16n(u32 addr, u16 value)
{
	data_cycles = data_tt[addr >> BUS_TIMING_SHIFT][2];
	store<u16>(this, addr, value);
}

void
arm7_cpu::store8n(u32 addr, u8 value)
{
	data_cycles = data_tt[addr >> BUS_TIMING_SHIFT][2];
	store<u8>(this, addr, value);
}

void
arm7_cpu::load_multiple(u32 addr, int count, u32 *values)
{
	data_cycles = load_store_multiple<1>(this, addr, count, values);
}

void
arm7_cpu::store_multiple(u32 addr, int count, u32 *values)
{
	data_cycles = load_store_multiple<0>(this, addr, count, values);
}

u16
arm7_cpu::ldrh(u32 addr)
{
	return std::rotr(load16n(addr & ~1), (addr & 1) << 3);
}

s16
arm7_cpu::ldrsh(u32 addr)
{
	if (addr & 1) {
		return (s8)load8n(addr);
	} else {
		return load16n(addr);
	}
}

void
arm7_cpu::ldm(u32 addr, u16 register_list, int count)
{
	if (register_list == 0) {
		register_list = 0x8000;
		count = 1;
	}

	u32 values[16]{};
	load_multiple(addr, count, values);

	u32 *p = values;
	p = arm::interpreter::ldm_loop(register_list, 15, p, gpr);

	add_ldr_cycles();

	if (register_list & 1) {
		arm_jump(*p & ~3);
	}
}

void
arm7_cpu::ldm_user(u32 addr, u16 register_list, int count)
{
	if (mode == MODE_SYS || mode == MODE_USR) {
		return ldm(addr, register_list, count);
	}

	if (register_list == 0) {
		register_list = 0x8000;
		count = 1;
	}

	u32 values[16]{};
	load_multiple(addr, count, values);

	u32 *p = values;
	p = arm::interpreter::ldm_loop(
			register_list, mode == MODE_FIQ ? 8 : 13, p, gpr);
	if (mode == MODE_FIQ) {
		p = arm::interpreter::ldm_loop(register_list, 5, p, fiqr);
	}
	p = arm::interpreter::ldm_loop(register_list, 2, p, bankedr[0]);

	add_ldr_cycles();

	if (register_list & 1) {
		arm_jump(*p & ~3);
	}
}

void
arm7_cpu::ldm_cpsr(u32 addr, u16 register_list, int count)
{
	u32 values[16]{};
	load_multiple(addr, count, values);

	u32 *p = values;
	p = arm::interpreter::ldm_loop(register_list, 15, p, gpr);

	add_ldr_cycles();

	cpsr = spsr();
	if (cpsr & 0x20) {
		thumb_jump(*p & ~1);
	} else {
		arm_jump(*p & ~3);
	}
}

void
arm7_cpu::stm(u32 addr, u16 register_list, int count)
{
	if (register_list == 0) {
		register_list = 0x8000;
		count = 1;
	}

	u32 values[16]{};

	u32 *p = values;
	p = arm::interpreter::stm_loop(register_list, 15, gpr, p);
	if (register_list & 1) {
		*p++ = pc() + 4;
	}

	store_multiple(addr, count, values);
	add_str_cycles();
}

void
arm7_cpu::stm_user(u32 addr, u16 register_list, int count)
{
	if (mode == MODE_SYS || mode == MODE_USR)
		return stm(addr, register_list, count);

	if (register_list == 0) {
		register_list = 0x8000;
		count = 1;
	}

	u32 values[16]{};

	u32 *p = values;
	p = arm::interpreter::stm_loop(
			register_list, mode == MODE_FIQ ? 8 : 13, gpr, p);
	if (mode == MODE_FIQ) {
		p = arm::interpreter::stm_loop(register_list, 5, fiqr, p);
	}
	p = arm::interpreter::stm_loop(register_list, 2, bankedr[0], p);
	if (register_list & 1) {
		*p++ = pc() + 4;
	}

	store_multiple(addr, count, values);
	add_str_cycles();
}

void
arm7_cpu::thumb_ldm(u32 addr, u16 register_list, int count)
{
	if (register_list == 0) {
		register_list = 0x100;
		count = 1;
	}

	u32 values[8]{};
	load_multiple(addr, count, values);

	u32 *p = values;
	p = arm::interpreter::ldm_loop(register_list, 8, p, gpr);

	add_ldr_cycles();

	if (register_list & 1) {
		thumb_jump(*p & ~1);
	}
}

void
arm7_cpu::thumb_stm(u32 addr, u16 register_list, int count)
{
	if (register_list == 0) {
		register_list = 0x100;
		count = 1;
	}

	u32 values[8]{};

	u32 *p = values;
	p = arm::interpreter::stm_loop(register_list, 8, gpr, p);
	if (register_list & 1) {
		*p++ = pc() + 2;
	}

	store_multiple(addr, count, values);
	add_str_cycles();
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
arm7_init_tables(arm7_cpu *cpu)
{
	cpu->read_pt = cpu->nds->bus7_read_pt;
	cpu->write_pt = cpu->nds->bus7_write_pt;
	cpu->code_tt = cpu->nds->arm7_code_timings;
	cpu->data_tt = cpu->nds->arm7_data_timings;
}

} // namespace twice
