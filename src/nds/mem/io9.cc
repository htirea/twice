#include "nds/mem/io.h"

#include "common/logger.h"
#include "nds/math.h"

namespace twice {

void
wramcnt_write(nds_ctx *nds, u8 value)
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
powcnt1_write(nds_ctx *nds, u16 value)
{
	if (value & BIT(15)) {
		nds->gpu2d[0].fb = nds->fb;
		nds->gpu2d[1].fb = nds->fb + NDS_SCREEN_SZ;
	} else {
		nds->gpu2d[0].fb = nds->fb + NDS_SCREEN_SZ;
		nds->gpu2d[1].fb = nds->fb;
	}

	/* TODO: check what bit 0 does */
	nds->gpu2d[0].enabled = value & BIT(1);
	nds->gpu2d[1].enabled = value & BIT(9);

	/* TODO: disable writes / reads if disabled */

	nds->powcnt1 = (nds->powcnt1 & ~0x820F) | (value & 0x820F);
}

u8
io9_read8(nds_ctx *nds, u32 addr)
{
	switch (addr) {
		IO_READ8_COMMON(0);
	case 0x4000247:
		return nds->wramcnt;
	}

	LOG("nds 0 read 8 at %08X\n", addr);
	return 0;
}

u16
io9_read16(nds_ctx *nds, u32 addr)
{
	switch (addr) {
		IO_READ16_COMMON(0);
	case 0x4000280:
		return nds->divcnt;
	case 0x40002B0:
		return nds->sqrtcnt;
	case 0x4000304:
		return nds->powcnt1;
	}

	if (0x4000008 <= addr && addr < 0x4000058) {
		return gpu_2d_read16(&nds->gpu2d[0], addr);
	}

	if (0x4001008 <= addr && addr < 0x4001058) {
		return gpu_2d_read16(&nds->gpu2d[1], addr);
	}

	LOG("nds 0 read 16 at %08X\n", addr);
	return 0;
}

u32
io9_read32(nds_ctx *nds, u32 addr)
{
	switch (addr) {
		IO_READ32_COMMON(0);
	case 0x4000000:
		return nds->gpu2d[0].dispcnt;
	case 0x40000E0:
		return nds->dmafill[0];
	case 0x40000E4:
		return nds->dmafill[1];
	case 0x40000E8:
		return nds->dmafill[2];
	case 0x40000EC:
		return nds->dmafill[3];
	case 0x4000240:
		return (u32)nds->vram.vramcnt[3] << 24 |
		       (u32)nds->vram.vramcnt[2] << 16 |
		       (u32)nds->vram.vramcnt[1] << 8 |
		       (u32)nds->vram.vramcnt[0];
	case 0x4000280:
		return nds->divcnt;
	case 0x4000290:
		return nds->div_numer[0];
	case 0x4000294:
		return nds->div_numer[1];
	case 0x4000298:
		return nds->div_denom[0];
	case 0x400029C:
		return nds->div_denom[1];
	case 0x40002A0:
		return nds->div_result[0];
	case 0x40002A4:
		return nds->div_result[1];
	case 0x40002A8:
		return nds->divrem_result[0];
	case 0x40002AC:
		return nds->divrem_result[1];
	case 0x40002B4:
		return nds->sqrt_result;
	case 0x40002B8:
		return nds->sqrt_param[0];
	case 0x40002BC:
		return nds->sqrt_param[1];
	case 0x4000304:
		return nds->powcnt1;
	case 0x4001000:
		return nds->gpu2d[1].dispcnt;
	}

	if (0x4000008 <= addr && addr < 0x4000058) {
		return gpu_2d_read32(&nds->gpu2d[0], addr);
	}

	if (0x4001008 <= addr && addr < 0x4001058) {
		return gpu_2d_read32(&nds->gpu2d[1], addr);
	}

	LOG("nds 0 read 32 at %08X\n", addr);
	return 0;
}

void
io9_write8(nds_ctx *nds, u32 addr, u8 value)
{
	switch (addr) {
		IO_WRITE8_COMMON(0);
	case 0x4000240:
		vramcnt_a_write(nds, value);
		return;
	case 0x4000241:
		vramcnt_b_write(nds, value);
		return;
	case 0x4000242:
		vramcnt_c_write(nds, value);
		return;
	case 0x4000243:
		vramcnt_d_write(nds, value);
		return;
	case 0x4000244:
		vramcnt_e_write(nds, value);
		return;
	case 0x4000245:
		vramcnt_f_write(nds, value);
		return;
	case 0x4000246:
		vramcnt_g_write(nds, value);
		return;
	case 0x4000247:
		wramcnt_write(nds, value);
		return;
	case 0x4000248:
		vramcnt_h_write(nds, value);
		return;
	case 0x4000249:
		vramcnt_i_write(nds, value);
		return;
	case 0x4000300:
		/* once bit 0 is set, cant be unset
		 * bit 1 is r/w
		 */
		nds->postflg[0] &= ~BIT(1);
		nds->postflg[0] |= value & 3;
		return;
	}

	LOG("nds 0 write 8 to %08X\n", addr);
}

void
io9_write16(nds_ctx *nds, u32 addr, u16 value)
{
	switch (addr) {
		IO_WRITE16_COMMON(0);
	case 0x4000280:
		/* TODO: div timings */
		nds->divcnt = (nds->divcnt & 0x8000) | (value & ~0x8000);
		nds_math_div(nds);
		return;
	case 0x40002B0:
		/* TODO: sqrt timings */
		nds->sqrtcnt = (nds->sqrtcnt & 0x8000) | (value & ~0x8000);
		nds_math_sqrt(nds);
		return;
	case 0x4000304:
		powcnt1_write(nds, value);
		return;
	}

	if (0x4000008 <= addr && addr < 0x4000058) {
		gpu_2d_write16(&nds->gpu2d[0], addr, value);
		return;
	}

	if (0x4001008 <= addr && addr < 0x4001058) {
		gpu_2d_write16(&nds->gpu2d[1], addr, value);
		return;
	}

	LOG("nds 0 write 16 to %08X\n", addr);
}

void
io9_write32(nds_ctx *nds, u32 addr, u32 value)
{
	switch (addr) {
		IO_WRITE32_COMMON(0);
	case 0x4000000:
		nds->gpu2d[0].dispcnt = value;
		return;
	case 0x40000E0:
		nds->dmafill[0] = value;
		return;
	case 0x40000E4:
		nds->dmafill[1] = value;
		return;
	case 0x40000E8:
		nds->dmafill[2] = value;
		return;
	case 0x40000EC:
		nds->dmafill[3] = value;
		return;
	case 0x4000240:
		vramcnt_a_write(nds, value);
		vramcnt_b_write(nds, value >> 8);
		vramcnt_c_write(nds, value >> 16);
		vramcnt_d_write(nds, value >> 24);
		return;
	case 0x4000244:
		vramcnt_e_write(nds, value);
		vramcnt_f_write(nds, value >> 8);
		vramcnt_g_write(nds, value >> 16);
		wramcnt_write(nds, value >> 24);
		return;
	case 0x4000280:
		nds->divcnt = (nds->divcnt & 0x8000) | (value & ~0x8000);
		nds_math_div(nds);
		return;
	case 0x4000290:
		nds->div_numer[0] = value;
		nds_math_div(nds);
		return;
	case 0x4000294:
		nds->div_numer[1] = value;
		nds_math_div(nds);
		return;
	case 0x4000298:
		nds->div_denom[0] = value;
		nds_math_div(nds);
		return;
	case 0x400029C:
		nds->div_denom[1] = value;
		nds_math_div(nds);
		return;
	case 0x40002B0:
		nds->sqrtcnt = (nds->sqrtcnt & 0x8000) | (value & ~0x8000);
		nds_math_sqrt(nds);
		return;
	case 0x40002B8:
		nds->sqrt_param[0] = value;
		nds_math_sqrt(nds);
		return;
	case 0x40002BC:
		nds->sqrt_param[1] = value;
		nds_math_sqrt(nds);
		return;
	case 0x4000304:
		powcnt1_write(nds, value);
		return;
	case 0x4001000:
		nds->gpu2d[1].dispcnt = value;
		return;
	case 0x4001058:
		return;
	}

	if (0x4000008 <= addr && addr < 0x4000058) {
		gpu_2d_write32(&nds->gpu2d[0], addr, value);
		return;
	}

	if (0x4001008 <= addr && addr < 0x4001058) {
		gpu_2d_write32(&nds->gpu2d[1], addr, value);
		return;
	}

	LOG("nds 0 write 32 to %08X\n", addr);
}

} // namespace twice
