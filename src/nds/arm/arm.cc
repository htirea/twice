#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

using namespace twice;

static bool
check_cond(u32 cpsr, u32 cond)
{
	if (cond == 0xE) {
		return true;
	}

	bool N = cpsr & (1 << 31);
	bool Z = cpsr & (1 << 30);
	bool C = cpsr & (1 << 29);
	bool V = cpsr & (1 << 28);

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

	fprintf(stderr, "unknown cond\n");
	return false;
}

void
Arm9::step()
{
	if (in_thumb()) {
		pc() += 2;
		opcode.word = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch16(pc());
		thumb_inst_lut[opcode.word >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode.word = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch32(pc());

		if (check_cond(cpsr, opcode.word >> 28)) {
			u32 op1 = opcode.word >> 20 & 0xFF;
			u32 op2 = opcode.word >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		} else if ((opcode.word & 0xFE000000) == 0xFA000000) {
			throw TwiceError("unimplemented blx1\n");
		}
	}

	nds->cycles += 1;
}

void
Arm7::step()
{
	if (in_thumb()) {
		pc() += 2;
		opcode.word = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch16(pc());
		thumb_inst_lut[opcode.word >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode.word = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch32(pc());

		if (check_cond(cpsr, opcode.word >> 28)) {
			u32 op1 = opcode.word >> 20 & 0xFF;
			u32 op2 = opcode.word >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		}
	}
}

void
Arm9::jump(u32 addr)
{
	if (in_thumb()) {
		pc() = addr + 2;
		pipeline[0] = fetch16(addr);
		pipeline[1] = fetch16(addr + 2);
	} else {
		pc() = addr + 4;
		pipeline[0] = fetch32(addr);
		pipeline[1] = fetch32(addr + 4);
	}
}

void
Arm7::jump(u32 addr)
{
	if (in_thumb()) {
		pc() = addr + 2;
		pipeline[0] = fetch16(addr);
		pipeline[1] = fetch16(addr + 2);
	} else {
		pc() = addr + 4;
		pipeline[0] = fetch32(addr);
		pipeline[1] = fetch32(addr + 4);
	}
}

template <typename T>
T
Arm9::fetch(u32 addr)
{
	if (read_itcm && 0 == (addr & itcm_addr_mask)) {
		return readarr<T>(itcm, addr & itcm_array_mask);
	}

	return bus9_read<T>(nds, addr);
}

template <typename T>
T
Arm9::load(u32 addr)
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
Arm9::store(u32 addr, T value)
{
	if (write_itcm && 0 == (addr & itcm_addr_mask)) {
		writearr<T>(itcm, addr & itcm_array_mask, value);
	}

	if (write_dtcm && dtcm_base == (addr & dtcm_addr_mask)) {
		writearr<T>(dtcm, addr & dtcm_array_mask, value);
	}

	bus9_write<T>(nds, addr, value);
}

u32
Arm9::fetch32(u32 addr)
{
	return fetch<u32>(addr);
}

u16
Arm9::fetch16(u32 addr)
{
	return fetch<u16>(addr);
}

u32
Arm9::load32(u32 addr)
{
	return load<u32>(addr);
}

u16
Arm9::load16(u32 addr)
{
	return load<u16>(addr);
}

u8
Arm9::load8(u32 addr)
{
	return load<u8>(addr);
}

void
Arm9::store32(u32 addr, u32 value)
{
	store<u32>(addr, value);
}

void
Arm9::store16(u32 addr, u16 value)
{
	store<u16>(addr, value);
}

void
Arm9::store8(u32 addr, u8 value)
{
	store<u8>(addr, value);
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

static void
ctrl_reg_write(Arm9 *cpu, u32 value)
{
	u32 diff = cpu->ctrl_reg ^ value;
	u32 unhandled = (1 << 0) | (1 << 2) | (1 << 7) | (1 << 12) |
			(1 << 14) | (1 << 15);

	if (diff & unhandled) {
		fprintf(stderr, "unhandled bits in cp15 write %08X\n", value);
	}

	cpu->exception_base = value & (1 << 13) ? 0xFFFF0000 : 0;

	if (diff & (1 << 16 | 1 << 17)) {
		cpu->read_dtcm = (value & 1 << 16) && !(value & 1 << 17);
		cpu->write_dtcm = value & 1 << 16;
	}

	if (diff & (1 << 18 | 1 << 19)) {
		cpu->read_itcm = (value & 1 << 18) && !(value & 1 << 19);
		cpu->write_itcm = value & 1 << 18;
	}

	cpu->ctrl_reg = (cpu->ctrl_reg & ~0xFF085) | (value & 0xFF085);
}

void
Arm9::cp15_write(u32 reg, u32 value)
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
	default:
		fprintf(stderr, "unhandled cp15 write to %03X\n", reg);
	}
}
