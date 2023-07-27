#include "nds/mem/io.h"

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

} // namespace twice
