#include "nds/arm/arm9.h"

#include "nds/arm/interpreter/lut.h"
#include "nds/arm/interpreter/util.h"
#include "nds/nds.h"

#include "common/logger.h"
#include "libtwice/exception.h"

namespace twice {

static void ctrl_reg_write(arm9_cpu *cpu, u32 value);
static void set_dtcm_params(arm9_cpu *cpu, u32 base, u32 mask);
static void set_itcm_params(arm9_cpu *cpu, u32 mask);
static void remap_fetch_pt(arm9_cpu *cpu, u64 unmap_start, u64 unmap_end);
static void remap_load_pt(arm9_cpu *cpu, u64 unmap_start, u64 unmap_end);
static void remap_store_pt(arm9_cpu *cpu, u64 unmap_start, u64 unmap_end);
static void unmap_tcm_pages(arm9_cpu *cpu, int table, u64 start, u64 end);
static void map_dtcm_pages(arm9_cpu *cpu, int table);
static void map_itcm_pages(arm9_cpu *cpu, int table);

void
arm9_cpu::run()
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
arm9_cpu::step()
{
	if (cpsr & 0x20) {
		pc() += 2;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		if (pc() & 2) {
			pipeline[1] >>= 16;
			code_cycles = 0;
		} else {
			code_cycles = fetch32n(pc(), &pipeline[1]);
		}
		thumb_inst_lut[opcode >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		code_cycles = fetch32n(pc(), &pipeline[1]);

		u32 cond = opcode >> 28;
		if (cond == 0xE ||
				arm_cond_table[cond] & (1 << (cpsr >> 28))) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		} else if (cond == 0xF) {
			if ((opcode & 0xFE000000) == 0xFA000000) {
				add_code_cycles();

				bool H = opcode & BIT(24);
				s32 offset = ((s32)(opcode << 8) >> 6) +
				             (H << 1);

				gpr[14] = pc() - 4;
				cpsr |= 0x20;
				thumb_jump(pc() + offset);
			} else if ((opcode & 0xFD700000) == 0xF5500000) {
				throw twice_error("PLD");
			} else if ((opcode & 0xFE000000) == 0xFC000000) {
				throw twice_error("STC2 / LDC2");
			} else if ((opcode & 0xFF000000) == 0xFE000000) {
				throw twice_error("CDP2 / MCR2 / MRC2");
			}
		} else {
			add_code_cycles();
		}
	}

	if (interrupt) {
		arm_do_irq(this);
	}
}

void
arm9_cpu::arm_jump(u32 addr)
{
	pc() = addr + 4;
	*cycles += fetch32n(addr, &pipeline[0]);
	*cycles += fetch32n(addr + 4, &pipeline[1]);
}

void
arm9_cpu::thumb_jump(u32 addr)
{
	pc() = addr + 2;
	if (addr & 2) {
		*cycles += fetch32n(addr - 2, &pipeline[0]);
		pipeline[0] >>= 16;
		*cycles += fetch32n(addr + 2, &pipeline[1]);
	} else {
		*cycles += fetch32n(addr, &pipeline[0]);
		pipeline[1] = pipeline[0] >> 16;
	}
}

void
arm9_cpu::jump_cpsr(u32 addr)
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
static u8
fetch(arm9_cpu *cpu, u32 addr, T *result)
{
	u8 *p = cpu->pages[arm9_cpu::FETCH][addr >> BUS9_PAGE_SHIFT];
	if (p) {
		*result = readarr<T>(p, addr & BUS9_PAGE_MASK);
	} else {
		*result = bus9_read_slow<T>(cpu->nds, addr);
	}

	return cpu->timings[arm9_cpu::FETCH][addr >> BUS9_PAGE_SHIFT][0];
}

template <typename T>
static T
load(arm9_cpu *cpu, u32 addr)
{
	u8 *p = cpu->pages[arm9_cpu::LOAD][addr >> BUS9_PAGE_SHIFT];
	if (p) {
		return readarr<T>(p, addr & BUS9_PAGE_MASK);
	}

	return bus9_read_slow<T>(cpu->nds, addr);
}

template <typename T>
static void
store(arm9_cpu *cpu, u32 addr, T value)
{
	u8 *p = cpu->pages[arm9_cpu::STORE][addr >> BUS9_PAGE_SHIFT];
	if (p) {
		writearr<T>(p, addr & BUS9_PAGE_MASK, value);
		return;
	}

	bus9_write_slow<T>(cpu->nds, addr, value);
}

static u32 *
load_multiple_from_page(
		arm9_cpu *cpu, u32 page, u32 addr, int count, u32 *values)
{
	u8 *p = cpu->pages[arm9_cpu::LOAD][page];
	if (p) {
		for (; count--; addr += 4) {
			*values++ = readarr<u32>(p, addr & BUS9_PAGE_MASK);
		}
	} else {
		for (; count--; addr += 4) {
			*values++ = bus9_read_slow<u32>(cpu->nds, addr);
		}
	}

	return values;
}

static u32 *
store_multiple_to_page(
		arm9_cpu *cpu, u32 page, u32 addr, int count, u32 *values)
{
	u8 *p = cpu->pages[arm9_cpu::STORE][page];
	if (p) {
		for (; count--; addr += 4) {
			writearr<u32>(p, addr & BUS9_PAGE_MASK, *values++);
		}
	} else {
		for (; count--; addr += 4) {
			bus9_write_slow<u32>(cpu->nds, addr, *values++);
		}
	}

	return values;
}

template <int Table>
static u32
load_store_multiple(arm9_cpu *cpu, u32 addr, int count, u32 *values)
{
	if (count == 0) {
		return 0;
	}

	u32 start_page = addr >> BUS9_PAGE_SHIFT;
	u32 end_page = (addr + 4 * (count - 1)) >> BUS9_PAGE_SHIFT;

	if (start_page == end_page) {
		if (Table == arm9_cpu::LOAD) {
			load_multiple_from_page(
					cpu, start_page, addr, count, values);
		} else {
			store_multiple_to_page(
					cpu, start_page, addr, count, values);
		}

		auto& t = cpu->timings[Table][addr >> BUS9_PAGE_SHIFT];
		return t[0] + (count - 1) * t[1];
	} else {
		u32 end_addr = end_page << BUS9_PAGE_SHIFT;
		int count1 = (end_addr - addr) >> 2;

		if (Table == arm9_cpu::LOAD) {
			values = load_multiple_from_page(
					cpu, start_page, addr, count1, values);
			load_multiple_from_page(cpu, end_page, 0,
					count - count1, values);
		} else {
			values = store_multiple_to_page(
					cpu, start_page, addr, count1, values);
			store_multiple_to_page(cpu, end_page, 0,
					count - count1, values);
		}

		auto& t1 = cpu->timings[Table][addr >> BUS9_PAGE_SHIFT];
		auto& t2 = cpu->timings[Table][end_addr >> BUS9_PAGE_SHIFT];
		return t1[0] + (count1 - 1) * t1[1] + (count - count1) * t2[1];
	}
}

u8
arm9_cpu::fetch32n(u32 addr, u32 *result)
{
	return fetch<u32>(this, addr, result);
}

u32
arm9_cpu::load32n(u32 addr)
{
	data_cycles = timings[LOAD][addr >> BUS9_PAGE_SHIFT][0];
	return load<u32>(this, addr);
}

u32
arm9_cpu::load32s(u32 addr)
{
	data_cycles += timings[LOAD][addr >> BUS9_PAGE_SHIFT][1];
	return load<u32>(this, addr);
}

u16
arm9_cpu::load16n(u32 addr)
{
	data_cycles = timings[LOAD][addr >> BUS9_PAGE_SHIFT][2];
	return load<u16>(this, addr);
}

u8
arm9_cpu::load8n(u32 addr)
{
	data_cycles = timings[LOAD][addr >> BUS9_PAGE_SHIFT][2];
	return load<u8>(this, addr);
}

void
arm9_cpu::store32n(u32 addr, u32 value)
{
	data_cycles = timings[STORE][addr >> BUS9_PAGE_SHIFT][0];
	store<u32>(this, addr, value);
}

void
arm9_cpu::store32s(u32 addr, u32 value)
{
	data_cycles = timings[STORE][addr >> BUS9_PAGE_SHIFT][1];
	store<u32>(this, addr, value);
}

void
arm9_cpu::store16n(u32 addr, u16 value)
{
	data_cycles = timings[STORE][addr >> BUS9_PAGE_SHIFT][2];
	store<u16>(this, addr, value);
}

void
arm9_cpu::store8n(u32 addr, u8 value)
{
	data_cycles = timings[STORE][addr >> BUS9_PAGE_SHIFT][2];
	store<u8>(this, addr, value);
}

void
arm9_cpu::load_multiple(u32 addr, int count, u32 *values)
{
	data_cycles = load_store_multiple<LOAD>(this, addr, count, values);
}

void
arm9_cpu::store_multiple(u32 addr, int count, u32 *values)
{
	data_cycles = load_store_multiple<STORE>(this, addr, count, values);
}

u16
arm9_cpu::ldrh(u32 addr)
{
	return load16n(addr & ~1);
}

s16
arm9_cpu::ldrsh(u32 addr)
{
	return load16n(addr & ~1);
}

void
arm9_cpu::ldm(u32 addr, u16 register_list, int count)
{
	u32 values[16]{};
	load_multiple(addr, count, values);

	u32 *p = values;
	p = arm::interpreter::ldm_loop(register_list, 15, p, gpr);

	add_ldr_cycles();

	if (register_list & 1) {
		arm::interpreter::arm_do_bx(this, *p);
	}
}

void
arm9_cpu::ldm_user(u32 addr, u16 register_list, int count)
{
	if (mode == MODE_SYS || mode == MODE_USR) {
		return ldm(addr, register_list, count);
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
}

void
arm9_cpu::ldm_cpsr(u32 addr, u16 register_list, int count)
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
arm9_cpu::stm(u32 addr, u16 register_list, int count)
{
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
arm9_cpu::stm_user(u32 addr, u16 register_list, int count)
{
	if (mode == MODE_SYS || mode == MODE_USR)
		return stm(addr, register_list, count);

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
arm9_cpu::thumb_ldm(u32 addr, u16 register_list, int count)
{
	u32 values[8]{};
	load_multiple(addr, count, values);

	u32 *p = values;
	arm::interpreter::ldm_loop(register_list, 8, p, gpr);

	add_ldr_cycles();
}

void
arm9_cpu::thumb_stm(u32 addr, u16 register_list, int count)
{
	u32 values[8]{};

	u32 *p = values;
	p = arm::interpreter::stm_loop(register_list, 8, gpr, p);

	store_multiple(addr, count, values);
	add_str_cycles();
}

void
arm9_direct_boot(arm9_cpu *cpu, u32 entry_addr)
{
	cp15_write(cpu, 0x100, 0x00012078);
	cp15_write(cpu, 0x910, 0x0300000A);
	cp15_write(cpu, 0x911, 0x00000020);
	cpu->gpr[12] = entry_addr;
	cpu->gpr[13] = 0x03002F7C;
	cpu->gpr[14] = entry_addr;
	cpu->bankedr[arm_cpu::MODE_IRQ][0] = 0x03003F80;
	cpu->bankedr[arm_cpu::MODE_SVC][0] = 0x03003FC0;
	cpu->arm_jump(entry_addr);
}

u32
cp15_read(arm9_cpu *cpu, u32 reg)
{
	switch (reg) {
	/* cache type register */
	case 0x001:
		return 0x0F0D2112;
	case 0x100:
		return cpu->ctrl_reg;
	/* protection unit */
	case 0x200:
	case 0x201:
	case 0x300:
	case 0x500:
	case 0x501:
	case 0x502:
	case 0x503:
	case 0x600:
	case 0x610:
	case 0x620:
	case 0x630:
	case 0x640:
	case 0x650:
	case 0x660:
	case 0x670:
		LOG("cp15 read %03X\n", reg);
		return 0;
	case 0x910:
		return cpu->dtcm_reg;
	case 0x911:
		return cpu->itcm_reg;
	default:
		LOG("cp15 read %03X\n", reg);
		throw twice_error("unhandled cp15 read");
	}
}

void
cp15_write(arm9_cpu *cpu, u32 reg, u32 value)
{
	switch (reg) {
	case 0x100:
		ctrl_reg_write(cpu, value);
		break;
	case 0x910:
	{
		u32 shift = std::clamp<u32>(value >> 1 & 0x1F, 3, 23);
		u32 mask = ((u64)512 << shift) - 1;
		set_dtcm_params(cpu, value & ~mask, mask);
		cpu->dtcm_reg = value & 0xFFFFF03E;
		break;
	}
	case 0x911:
	{
		u32 shift = std::clamp<u32>(value >> 1 & 0x1F, 3, 23);
		u32 mask = ((u64)512 << shift) - 1;
		set_itcm_params(cpu, mask);
		cpu->itcm_reg = value & 0x3E;
		break;
	}
	case 0x704:
	case 0x782:
		halt_cpu(cpu, arm_cpu::CPU_HALT);
		break;
	case 0x200:
	case 0x201:
	case 0x300:
	case 0x500:
	case 0x501:
	case 0x502:
	case 0x503:
	case 0x600:
	case 0x610:
	case 0x620:
	case 0x630:
	case 0x640:
	case 0x650:
	case 0x660:
	case 0x670:
	case 0x601:
	case 0x611:
	case 0x621:
	case 0x631:
	case 0x641:
	case 0x651:
	case 0x661:
	case 0x671:
		LOGVV("[protection_unit] unhandled cp15 write to %03X\n", reg);
		break;
	case 0x750:
	case 0x751:
	case 0x752:
	case 0x754:
	case 0x756:
	case 0x757:
	case 0x760:
	case 0x761:
	case 0x762:
	case 0x770:
	case 0x771:
	case 0x772:
	case 0x7A1:
	case 0x7A2:
	case 0x7A4:
	case 0x7B1:
	case 0x7B2:
	case 0x7D1:
	case 0x7E1:
	case 0x7E2:
	case 0x7F1:
	case 0x7F2:
		LOGVV("[cache_control] unhandled cp15 write to %03X\n", reg);
		break;
	default:
		LOG("unhandled cp15 write to %03X\n", reg);
	}
}

void
update_arm9_page_tables(arm9_cpu *cpu, u64 start, u64 end)
{
	LOG("remapping %09lX %09lX\n", start, end);

	remap_fetch_pt(cpu, start, end);
	remap_load_pt(cpu, start, end);
	remap_store_pt(cpu, start, end);
}

void
update_arm9_timing_tables(arm9_cpu *cpu, u64 start, u64 end)
{
}

static void
ctrl_reg_write(arm9_cpu *cpu, u32 value)
{
	u32 diff = cpu->ctrl_reg ^ value;
	u32 unhandled = BIT(0) | BIT(2) | BIT(7) | BIT(12) | BIT(14) | BIT(15);

	if (diff & unhandled) {
		LOG("unhandled bits in cp15 write %08X\n", value);
	}

	cpu->exception_base = value & BIT(13) ? 0xFFFF0000 : 0;

	bool tcm_changed = false;

	if (diff & (BIT(16) | BIT(17))) {
		cpu->read_dtcm = (value & BIT(16)) && !(value & BIT(17));
		cpu->write_dtcm = value & BIT(16);
		tcm_changed = true;
	}

	if (diff & (BIT(18) | BIT(19))) {
		cpu->read_itcm = (value & BIT(18)) && !(value & BIT(19));
		cpu->write_itcm = value & BIT(18);
		tcm_changed = true;
	}

	cpu->ctrl_reg = (cpu->ctrl_reg & ~0xFF085) | (value & 0xFF085);

	if (tcm_changed) {
		update_arm9_page_tables(cpu, 0, 4_GiB);
	}
}

static void
set_dtcm_params(arm9_cpu *cpu, u32 dtcm_base, u32 mask)
{
	u64 old_dtcm_base = cpu->dtcm_base;
	u64 old_dtcm_end = cpu->dtcm_end;
	cpu->dtcm_base = dtcm_base;
	cpu->dtcm_end = dtcm_base + (u64)mask + 1;
	cpu->dtcm_array_mask = mask & arm9_cpu::DTCM_MASK;

	if (cpu->read_dtcm) {
		remap_load_pt(cpu, old_dtcm_base, old_dtcm_end);
	}

	if (cpu->write_dtcm) {
		remap_store_pt(cpu, old_dtcm_base, old_dtcm_end);
	}
}

static void
set_itcm_params(arm9_cpu *cpu, u32 mask)
{
	u64 old_itcm_end = (u64)cpu->itcm_end;
	cpu->itcm_end = (u64)mask + 1;
	cpu->itcm_array_mask = mask & arm9_cpu::ITCM_MASK;

	if (cpu->read_itcm) {
		remap_load_pt(cpu, 0, old_itcm_end);
		remap_fetch_pt(cpu, 0, old_itcm_end);
	}

	if (cpu->write_itcm) {
		remap_store_pt(cpu, 0, old_itcm_end);
	}
}

static void
remap_fetch_pt(arm9_cpu *cpu, u64 unmap_start, u64 unmap_end)
{
	unmap_tcm_pages(cpu, arm9_cpu::FETCH, unmap_start, unmap_end);

	if (cpu->read_itcm) {
		map_itcm_pages(cpu, arm9_cpu::FETCH);
	}
}

static void
remap_load_pt(arm9_cpu *cpu, u64 unmap_start, u64 unmap_end)
{
	unmap_tcm_pages(cpu, arm9_cpu::LOAD, unmap_start, unmap_end);

	if (cpu->read_dtcm) {
		map_dtcm_pages(cpu, arm9_cpu::LOAD);
	}

	if (cpu->read_itcm) {
		map_itcm_pages(cpu, arm9_cpu::LOAD);
	}
}

static void
remap_store_pt(arm9_cpu *cpu, u64 unmap_start, u64 unmap_end)
{
	unmap_tcm_pages(cpu, arm9_cpu::STORE, unmap_start, unmap_end);

	if (cpu->write_dtcm) {
		map_dtcm_pages(cpu, arm9_cpu::STORE);
	}

	if (cpu->write_itcm) {
		map_itcm_pages(cpu, arm9_cpu::STORE);
	}
}

static void
unmap_tcm_pages(arm9_cpu *cpu, int table, u64 start, u64 end)
{
	auto& pt = cpu->pages[table];
	auto& pt_src = table == arm9_cpu::STORE ? cpu->nds->bus9_write_pt
	                                        : cpu->nds->bus9_read_pt;
	auto& tt = cpu->timings[table];
	auto& tt_src = table == arm9_cpu::FETCH ? cpu->nds->arm9_code_timings
	                                        : cpu->nds->arm9_data_timings;

	for (u64 addr = start; addr < end; addr += BUS9_PAGE_SIZE) {
		u32 page = addr >> BUS9_PAGE_SHIFT;
		u32 tt_src_page = addr >> BUS_TIMING_SHIFT;
		pt[page] = pt_src[page];
		tt[page] = tt_src[tt_src_page];
	}
}

static void
map_dtcm_pages(arm9_cpu *cpu, int table)
{
	auto& pt = cpu->pages[table];
	auto& tt = cpu->timings[table];

	for (u64 addr = cpu->dtcm_base; addr < cpu->dtcm_end;
			addr += BUS9_PAGE_SIZE) {
		u32 page = addr >> BUS9_PAGE_SHIFT;
		pt[page] = &cpu->dtcm[addr & cpu->dtcm_array_mask];
		tt[page] = { 1, 1, 1, 1 };
	}
}

static void
map_itcm_pages(arm9_cpu *cpu, int table)
{
	auto& pt = cpu->pages[table];
	auto& tt = cpu->timings[table];

	for (u64 addr = 0; addr < cpu->itcm_end; addr += BUS9_PAGE_SIZE) {
		u32 page = addr >> BUS9_PAGE_SHIFT;
		pt[page] = &cpu->itcm[addr & cpu->itcm_array_mask];
		tt[page] = { 1, 1, 1, 1 };
	}
}

} // namespace twice
