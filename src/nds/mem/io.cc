#include "nds/mem/io.h"

#include "common/logger.h"

#include "libtwice/exception.h"

namespace twice {

void
ipcsync_write(nds_ctx *nds, int cpuid, u16 value)
{
	nds->ipcsync[cpuid ^ 1] &= ~0xF;
	nds->ipcsync[cpuid ^ 1] |= value >> 8 & 0xF;
	nds->ipcsync[cpuid] &= ~0x4F00;
	nds->ipcsync[cpuid] |= value & 0x4F00;

	if ((value & BIT(13)) && (nds->ipcsync[cpuid ^ 1] & BIT(14))) {
		request_interrupt(nds->cpu[cpuid ^ 1], 16);
	}
}

void
exmem_write(nds_ctx *nds, int cpuid, u16 value)
{
	nds->exmem[cpuid] = (nds->exmem[cpuid] & ~0x3F) | (value & 0x3F);
	if (cpuid == 0) {
		nds->exmem[0] = (nds->exmem[0] & ~0x8880) | (value & 0x8880);
		nds->exmem[1] = (nds->exmem[1] & ~0x8880) | (value & 0x8880);
		nds->gba_slot_cpu = nds->exmem[0] >> 7 & 1;
		nds->nds_slot_cpu = nds->exmem[0] >> 11 & 1;
	}
}

void
auxspicnt_write_l(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu) return;

	nds->auxspicnt = (nds->auxspicnt & ~0x43) | (value & 0x43);
}

void
auxspicnt_write_h(nds_ctx *nds, int cpuid, u8 value)
{
	if (cpuid != nds->nds_slot_cpu) return;

	nds->auxspicnt = (nds->auxspicnt & ~0xE000) |
	                 ((u16)value << 8 & 0xE000);
}

void
auxspicnt_write(nds_ctx *nds, int cpuid, u16 value)
{
	if (cpuid != nds->nds_slot_cpu) return;

	nds->auxspicnt = (nds->auxspicnt & ~0xE043) | (value & 0xE043);
}

void
auxspidata_write(nds_ctx *nds, int cpuid, u8 value)
{
	LOGV("auxspidata write value %02X\n", value);
}

void
romctrl_write(nds_ctx *nds, int cpuid, u32 value)
{
	if (cpuid != nds->nds_slot_cpu) return;

	nds->romctrl = (nds->romctrl & (BIT(23) | BIT(29))) |
	               (value & ~BIT(23));
	if (nds->romctrl & BIT(31)) {
		throw twice_error("romctrl start transfer");
	}
}

} // namespace twice
