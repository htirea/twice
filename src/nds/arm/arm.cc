#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/arm/interpreter/lut.h"
#include "nds/mem/bus.h"

#include "libtwice/exception.h"

namespace twice {

Arm9::Arm9(NDS *nds)
	: Arm(nds)
{
	cpuid = 0;
}

Arm7::Arm7(NDS *nds)
	: Arm(nds)
{
	cpuid = 1;
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

	throw TwiceException("invalid mode bits");
}

void
Arm::swap_registers(u32 old_mode, u32 new_mode)
{
	static_assert((MODE_SYS & 7) == (MODE_USR & 7));
	old_mode &= 7;
	new_mode &= 7;

	if (new_mode == old_mode) {
		return;
	}

	bankedr[old_mode][0] = gpr[13];
	bankedr[old_mode][1] = gpr[14];
	bankedr[old_mode][2] = bankedr[0][2];

	gpr[13] = bankedr[new_mode][0];
	gpr[14] = bankedr[new_mode][1];
	bankedr[0][2] = bankedr[new_mode][2];

	if (new_mode == MODE_FIQ || old_mode == MODE_FIQ) {
		for (int i = 0; i < 5; i++) {
			std::swap(gpr[8 + i], fiqr[i]);
		}
	}
}

void
Arm::switch_mode(u32 new_mode)
{
	if (new_mode != mode) {
		swap_registers(mode, new_mode);
	}

	mode = new_mode;
}

void
Arm::on_cpsr_write()
{
	u32 new_mode = mode_bits_to_mode(cpsr & 0x1F);

	switch_mode(new_mode);
}

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
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch16(pc());
		thumb_inst_lut[opcode >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch32(pc());

		if (check_cond(cpsr, opcode >> 28)) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		} else if ((opcode & 0xFE000000) == 0xFA000000) {
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
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch16(pc());
		thumb_inst_lut[opcode >> 6 & 0x3FF](this);
	} else {
		pc() += 4;
		opcode = pipeline[0];
		pipeline[0] = pipeline[1];
		pipeline[1] = fetch32(pc());

		if (check_cond(cpsr, opcode >> 28)) {
			u32 op1 = opcode >> 20 & 0xFF;
			u32 op2 = opcode >> 4 & 0xF;
			arm_inst_lut[op1 << 4 | op2](this);
		}
	}
}

void
Arm9::jump(u32 addr)
{
	if (in_thumb()) {
		thumb_jump(addr);
	} else {
		arm_jump(addr);
	}
}

void
Arm9::arm_jump(u32 addr)
{
	pc() = addr + 4;
	pipeline[0] = fetch32(addr);
	pipeline[1] = fetch32(addr + 4);
}

void
Arm9::thumb_jump(u32 addr)
{
	pc() = addr + 2;
	pipeline[0] = fetch16(addr);
	pipeline[1] = fetch16(addr + 2);
}

void
Arm9::jump_cpsr(u32 addr)
{
	cpsr = spsr();
	on_cpsr_write();

	if (in_thumb()) {
		thumb_jump(addr & ~1);
	} else {
		arm_jump(addr & ~3);
	}
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
		return;
	}

	if (write_dtcm && dtcm_base == (addr & dtcm_addr_mask)) {
		writearr<T>(dtcm, addr & dtcm_array_mask, value);
		return;
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

u16
Arm9::ldrh(u32 addr)
{
	return load16(addr & ~1);
}

u16
Arm7::ldrh(u32 addr)
{
	return std::rotr(load16(addr & ~1), (addr & 1) << 3);
}

s16
Arm9::ldrsh(u32 addr)
{
	return load16(addr & ~1);
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

u32
Arm9::cp15_read(u32 reg)
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
		fprintf(stderr, "cp15 read %03X\n", reg);
		return 0;
	case 0x910:
		return dtcm_reg;
	case 0x911:
		return itcm_reg;
	default:
		fprintf(stderr, "cp15 read %03X\n", reg);
		throw TwiceError("unhandled cp15 read");
	}
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

} // namespace twice
