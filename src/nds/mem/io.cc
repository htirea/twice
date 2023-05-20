#include "nds/mem/io.h"

namespace twice {

void
wramcnt_write(NDS *nds, u8 value)
{
	nds->wramcnt = value;

	switch (value & 3) {
	case 0:
		nds->shared_wram_p[0] = nds->shared_wram;
		nds->shared_wram_mask[0] = 32_KiB - 1;
		nds->shared_wram_p[1] = nds->arm7_wram;
		nds->shared_wram_mask[1] = ARM7_WRAM_MASK;
		break;
	case 1:
		nds->shared_wram_p[0] = nds->shared_wram + 16_KiB;
		nds->shared_wram_mask[0] = 16_KiB - 1;
		nds->shared_wram_p[1] = nds->shared_wram;
		nds->shared_wram_mask[1] = 16_KiB - 1;
		break;
	case 2:
		nds->shared_wram_p[0] = nds->shared_wram;
		nds->shared_wram_mask[0] = 16_KiB - 1;
		nds->shared_wram_p[1] = nds->shared_wram + 16_KiB;
		nds->shared_wram_mask[1] = 16_KiB - 1;
		break;
	case 3:
		nds->shared_wram_p[0] = nds->shared_wram_null;
		nds->shared_wram_mask[0] = 0;
		nds->shared_wram_p[1] = nds->shared_wram;
		nds->shared_wram_mask[1] = 32_KiB - 1;
	}
}

void
ipcsync_write(NDS *nds, int cpuid, u16 value)
{
	nds->ipcsync[cpuid ^ 1] &= ~0xF;
	nds->ipcsync[cpuid ^ 1] |= value >> 8 & 0xF;
	nds->ipcsync[cpuid] &= ~0x4F00;
	nds->ipcsync[cpuid] |= value & 0x4F00;

	if ((value & BIT(13)) && (nds->ipcsync[cpuid ^ 1] & BIT(14))) {
		nds->cpu[cpuid ^ 1]->request_interrupt(16);
	}
}

void
powcnt1_write(NDS *nds, u16 value)
{
	if (value & BIT(15)) {
		nds->gpu2D[0].fb = nds->fb;
		nds->gpu2D[1].fb = nds->fb + NDS_SCREEN_SZ;
	} else {
		nds->gpu2D[0].fb = nds->fb + NDS_SCREEN_SZ;
		nds->gpu2D[1].fb = nds->fb;
	}

	/* TODO: check what bit 0 does */
	nds->gpu2D[0].enabled = value & BIT(1);
	nds->gpu2D[1].enabled = value & BIT(9);

	/* TODO: disable writes / reads if disabled */

	nds->powcnt1 = (nds->powcnt1 & ~0x820F) | (value & 0x820F);
}

} // namespace twice
