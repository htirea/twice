#include "nds/arm/arm9.h"

#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "common/logger.h"
#include "libtwice/exception.h"

namespace twice {

arm9_cpu::arm9_cpu(nds_ctx *nds)
	: arm_cpu(nds, 0)
{
}

static bool
check_cond(u32 cond, u32 cpsr)
{
	return arm_cond_table[cond] & (1 << (cpsr >> 28));
}

void
arm9_cpu::step()
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
		} else if ((opcode & 0xFE000000) == 0xFA000000) {
			bool H = opcode & BIT(24);
			s32 offset = ((s32)(opcode << 8) >> 6) + (H << 1);

			gpr[14] = pc() - 4;
			cpsr |= 0x20;
			thumb_jump(pc() + offset);
		}
	}

	if (interrupt) {
		arm_do_irq(this);
	}

	cycles += 1;
}

void
arm9_cpu::arm_jump(u32 addr)
{
	pc() = addr + 4;
	pipeline[0] = fetch32(addr);
	pipeline[1] = fetch32(addr + 4);
}

void
arm9_cpu::thumb_jump(u32 addr)
{
	pc() = addr + 2;
	pipeline[0] = fetch16(addr);
	pipeline[1] = fetch16(addr + 2);
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
T
arm9_cpu::fetch(u32 addr)
{
	if (read_itcm && 0 == (addr & itcm_addr_mask)) {
		return readarr<T>(itcm, addr & itcm_array_mask);
	}

	return bus9_read<T>(nds, addr);
}

template <typename T>
T
arm9_cpu::load(u32 addr)
{
	if (read_itcm && 0 == (addr & itcm_addr_mask)) {
		return readarr<T>(itcm, addr & itcm_array_mask);
	}

	if (read_dtcm && dtcm_base == (addr & dtcm_addr_mask)) {
		return readarr<T>(dtcm, addr & dtcm_array_mask);
	}

	return bus9_read<T>(nds, addr);
}

template <typename T>
void
arm9_cpu::store(u32 addr, T value)
{
	if (write_itcm && 0 == (addr & itcm_addr_mask)) {
		writearr<T>(itcm, addr & itcm_array_mask, value);
		return;
	}

	if (write_dtcm && dtcm_base == (addr & dtcm_addr_mask)) {
		writearr<T>(dtcm, addr & dtcm_array_mask, value);
		return;
	}

	bus9_write<T>(nds, addr, value);
}

u32
arm9_cpu::fetch32(u32 addr)
{
	return fetch<u32>(addr);
}

u16
arm9_cpu::fetch16(u32 addr)
{
	return fetch<u16>(addr);
}

u32
arm9_cpu::load32(u32 addr)
{
	return load<u32>(addr);
}

u16
arm9_cpu::load16(u32 addr)
{
	return load<u16>(addr);
}

u8
arm9_cpu::load8(u32 addr)
{
	return load<u8>(addr);
}

void
arm9_cpu::store32(u32 addr, u32 value)
{
	store<u32>(addr, value);
}

void
arm9_cpu::store16(u32 addr, u16 value)
{
	store<u16>(addr, value);
}

void
arm9_cpu::store8(u32 addr, u8 value)
{
	store<u8>(addr, value);
}

u16
arm9_cpu::ldrh(u32 addr)
{
	return load16(addr & ~1);
}

s16
arm9_cpu::ldrsh(u32 addr)
{
	return load16(addr & ~1);
}

u32
arm9_cpu::cp15_read(u32 reg)
{
	switch (reg) {
	/* cache type register */
	case 0x001:
		return 0x0F0D2112;
	case 0x100:
		return ctrl_reg;
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
		return dtcm_reg;
	case 0x911:
		return itcm_reg;
	default:
		LOG("cp15 read %03X\n", reg);
		throw twice_error("unhandled cp15 read");
	}
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

	if (diff & (BIT(16) | BIT(17))) {
		cpu->read_dtcm = (value & BIT(16)) && !(value & BIT(17));
		cpu->write_dtcm = value & BIT(16);
	}

	if (diff & (BIT(18) | BIT(19))) {
		cpu->read_itcm = (value & BIT(18)) && !(value & BIT(19));
		cpu->write_itcm = value & BIT(18);
	}

	cpu->ctrl_reg = (cpu->ctrl_reg & ~0xFF085) | (value & 0xFF085);
}

void
arm9_cpu::cp15_write(u32 reg, u32 value)
{
	switch (reg) {
	case 0x100:
		ctrl_reg_write(this, value);
		break;
	case 0x910:
	{
		u32 shift = std::clamp<u32>(value >> 1 & 0x1F, 3, 23);
		u32 mask = ((u64)512 << shift) - 1;
		dtcm_base = value & ~mask;
		dtcm_addr_mask = ~mask;
		dtcm_array_mask = mask & DTCM_MASK;
		dtcm_reg = value & 0xFFFFF03E;
		break;
	}
	case 0x911:
	{
		u32 shift = std::clamp<u32>(value >> 1 & 0x1F, 3, 23);
		u32 mask = ((u64)512 << shift) - 1;
		itcm_addr_mask = ~mask;
		itcm_array_mask = mask & ITCM_MASK;
		itcm_reg = value & 0x3E;
		break;
	}
	case 0x704:
		halted = true;
		force_stop_cpu(this);
		break;
	case 0x751:
	case 0x7A4:
	case 0x7E1:
		LOGVV("unhandled cp15 write to %03X\n", reg);
		break;
	default:
		LOG("unhandled cp15 write to %03X\n", reg);
	}
}

} // namespace twice
