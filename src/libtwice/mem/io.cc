#include "libtwice/mem/io.h"
#include "libtwice/nds.h"

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

} // namespace twice
