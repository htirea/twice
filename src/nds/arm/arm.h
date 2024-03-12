#ifndef TWICE_ARM_H
#define TWICE_ARM_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct nds_ctx;

struct arm_cpu {
	virtual ~arm_cpu();

	enum cpu_mode {
		MODE_SYS,
		MODE_FIQ,
		MODE_SVC,
		MODE_ABT,
		MODE_IRQ,
		MODE_UND,
		MODE_USR = 8
	};

	enum halt_mode : u32 {
		CPU_HALT = BIT(0),
		CPU_STOP = BIT(30),
		CPU_GXFIFO_HALT = BIT(31),
	};

	u32 gpr[16]{};
	u32 bankedr[6][3]{};
	u32 fiqr[5]{};
	u32 cpsr{ 0x9F };
	u32 opcode;
	u32 pipeline[2]{};
	u32 exception_base{};
	u32 mode{ MODE_SYS };

	u32 IME{};
	u32 IF{};
	u32 IE{};
	bool interrupt{};
	u32 halted{};

	nds_ctx *nds{};
	int cpuid{};

	timestamp *cycles{};
	timestamp *target_cycles{};
	u32 cycles_executed{};
	u8 code_cycles{};
	u8 data_cycles{};

	u32& pc() { return gpr[15]; }

	u32& spsr() { return bankedr[0][2]; }

	bool stopped() { return halted & (CPU_STOP | CPU_GXFIFO_HALT); }

	void add_code_cycles(u32 extra = 0)
	{
		u32 x = code_cycles + extra;
		*cycles += x;
		cycles_executed += x;
	}

	virtual void add_ldr_cycles();
	virtual void add_str_cycles(u32 extra = 0);

	virtual void run() = 0;
	virtual void step() = 0;
	virtual void arm_jump(u32 addr) = 0;
	virtual void thumb_jump(u32 addr) = 0;
	virtual void jump_cpsr(u32 addr) = 0;
	virtual u32 load32n(u32 addr) = 0;
	virtual u32 load32s(u32 addr) = 0;
	virtual u16 load16n(u32 addr) = 0;
	virtual u8 load8n(u32 addr) = 0;
	virtual void store32n(u32 addr, u32 value) = 0;
	virtual void store32s(u32 addr, u32 value) = 0;
	virtual void store16n(u32 addr, u16 value) = 0;
	virtual void store8n(u32 addr, u8 value) = 0;
	virtual void load_multiple(u32 addr, int count, u32 *values) = 0;
	virtual void store_multiple(u32 addr, int count, u32 *values) = 0;
	virtual u16 ldrh(u32 addr) = 0;
	virtual s16 ldrsh(u32 addr) = 0;
	virtual void ldm(u32 addr, u16 register_list, int count) = 0;
	virtual void ldm_user(u32 addr, u16 register_list, int count) = 0;
	virtual void ldm_cpsr(u32 addr, u16 register_list, int count) = 0;
	virtual void stm(u32 addr, u16 register_list, int count) = 0;
	virtual void stm_user(u32 addr, u16 register_list, int count) = 0;
	virtual void thumb_ldm(u32 addr, u16 register_list, int count) = 0;
	virtual void thumb_stm(u32 addr, u16 register_list, int count) = 0;
	virtual bool check_halted() = 0;
};

void arm_init(nds_ctx *nds, int cpuid);
void arm_switch_mode(arm_cpu *cpu, u32 new_mode);
void arm_check_interrupt(arm_cpu *cpu);
void arm_on_cpsr_write(arm_cpu *cpu);
void arm_do_irq(arm_cpu *cpu);
void halt_cpu(arm_cpu *cpu, u32 halt_bits);
void unhalt_cpu(arm_cpu *cpu, u32 halt_bits);
void request_interrupt(arm_cpu *cpu, int bit);

extern const u16 arm_cond_table[16];

} // namespace twice

#endif
