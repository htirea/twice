#include "nds/mem/io.h"

#include "nds/rtc.h"
#include "nds/spi.h"

#include "common/logger.h"

namespace twice {

u8
io7_read8(nds_ctx *nds, u32 addr)
{
	switch (addr) {
		IO_READ8_COMMON(1);
	case 0x4000138:
		return rtc_read(nds);
	case 0x40001C2:
		return spidata_read(nds);
	case 0x4000240:
		return nds->vramstat;
	case 0x4000241:
		return nds->wramcnt;
	case 0x4000500:
		return nds->soundcnt;
	case 0x4000501:
		return nds->soundcnt >> 8;
	case 0x4000508:
		return nds->sound_cap_ch[0].cnt;
	case 0x4000509:
		return nds->sound_cap_ch[1].cnt;
	}

	if (0x4000400 <= addr && addr < 0x4000500) {
		return sound_read8(nds, addr);
	}

	LOG("nds 1 read 8 at %08X\n", addr);
	return 0;
}

u16
io7_read16(nds_ctx *nds, u32 addr)
{
	switch (addr) {
		IO_READ16_COMMON(1);
	case 0x4000134:
		return nds->rcnt;
	case 0x4000136:
		return nds->extkeyin;
	case 0x4000138:
		return rtc_read(nds);
	case 0x40001C0:
		return nds->spicnt;
	case 0x40001C2:
		return spidata_read(nds);
	case 0x4000304:
		return nds->powcnt2;
	case 0x4000504:
		return nds->soundbias;
	case 0x4000508:
		return (u16)nds->sound_cap_ch[1].cnt << 8 |
		       nds->sound_cap_ch[0].cnt;
	}

	if (0x4000400 <= addr && addr < 0x4000500) {
		return sound_read16(nds, addr);
	}

	LOG("nds 1 read 16 at %08X\n", addr);
	return 0;
}

u32
io7_read32(nds_ctx *nds, u32 addr)
{
	switch (addr) {
		IO_READ32_COMMON(1);
	case 0x40001C0:
		return (u32)spidata_read(nds) << 16 | nds->spicnt;
	}

	if (0x4000400 <= addr && addr < 0x4000500) {
		return sound_read32(nds, addr);
	}

	LOG("nds 1 read 32 at %08X\n", addr);
	return 0;
}

void
io7_write8(nds_ctx *nds, u32 addr, u8 value)
{
	switch (addr) {
		IO_WRITE8_COMMON(1);
	case 0x4000138:
		rtc_write(nds, value);
		return;
	case 0x40001C2:
		spidata_write(nds, value);
		return;
	case 0x4000300:
		if (nds->cpu[1]->gpr[15] < ARM7_BIOS_SIZE) {
			/* once bit 0 is set, cant be unset */
			nds->postflg[1] |= value & 1;
		}
		return;
	case 0x4000301:
		switch (value >> 6 & 0x3) {
		case 2:
			halt_cpu(nds->cpu[1], arm_cpu::CPU_HALT);
			break;
		case 3:
			halt_cpu(nds->cpu[1], arm_cpu::CPU_STOP);
			break;
		default:
			LOG("unhandled haltcnt\n");
		}
		nds->haltcnt = value;
		return;
	case 0x4000500:
		nds->soundcnt = (nds->soundcnt & ~0x7F) | (value & 0x7F);
		return;
	case 0x4000501:
		nds->soundcnt = (nds->soundcnt & ~0xBF00) |
		                ((u16)value << 8 & 0xBF00);
		return;
	case 0x4000508:
		sound_capture_write_cnt(nds, 0, value);
		return;
	case 0x4000509:
		sound_capture_write_cnt(nds, 1, value);
		return;
	}

	if (0x4000400 <= addr && addr < 0x4000500) {
		sound_write8(nds, addr, value);
		return;
	}

	LOG("nds 1 write 8 to %08X\n", addr);
}

void
io7_write16(nds_ctx *nds, u32 addr, u16 value)
{
	switch (addr) {
		IO_WRITE16_COMMON(1);
	case 0x4000134:
		nds->rcnt = value;
		return;
	case 0x4000138:
		rtc_write(nds, value);
		return;
	case 0x40001C0:
		spicnt_write(nds, value);
		return;
	case 0x40001C2:
		spidata_write(nds, value);
		return;
	case 0x4000206:
		if (nds->powcnt2 & BIT(1)) {
			nds->wifiwaitcnt = value & 0x3F;
		}
		return;
	case 0x4000300:
		if (nds->cpu[1]->gpr[15] < ARM7_BIOS_SIZE) {
			nds->postflg[1] |= value & 1;
		}
		return;
	case 0x4000304:
		nds->powcnt2 = value & 0x3;
		return;
	case 0x4000500:
		nds->soundcnt = (nds->soundcnt & ~0xBF7F) | (value & 0xBF7F);
		return;
	case 0x4000504:
		nds->soundbias = value & 0x3FF;
		return;
	case 0x4000508:
		sound_capture_write_cnt(nds, 0, value);
		sound_capture_write_cnt(nds, 1, value >> 8);
		return;
	case 0x4000514:
		nds->sound_cap_ch[0].len = value;
		return;
	case 0x400051C:
		nds->sound_cap_ch[1].len = value;
		return;
	}

	if (0x4000400 <= addr && addr < 0x4000500) {
		sound_write16(nds, addr, value);
		return;
	}

	LOG("nds 1 write 16 to %08X\n", addr);
}

void
io7_write32(nds_ctx *nds, u32 addr, u32 value)
{
	switch (addr) {
		IO_WRITE32_COMMON(1);
	case 0x4000138:
		rtc_write(nds, value);
		return;
	case 0x4000500:
		nds->soundcnt = (nds->soundcnt & ~0xBF7F) | (value & 0xBF7F);
		return;
	case 0x4000508:
		sound_capture_write_cnt(nds, 0, value);
		sound_capture_write_cnt(nds, 1, value >> 8);
		return;
	case 0x4000510:
		nds->sound_cap_ch[0].dad = value & 0x7FFFFFC;
		return;
	case 0x4000514:
		nds->sound_cap_ch[0].len = value & 0xFFFF;
		return;
	case 0x400051C:
		nds->sound_cap_ch[1].len = value & 0xFFFF;
		return;
	case 0x4000518:
		nds->sound_cap_ch[1].dad = value & 0x7FFFFFC;
		return;
	}

	if (0x4000400 <= addr && addr < 0x4000500) {
		sound_write32(nds, addr, value);
		return;
	}

	LOG("nds 1 write 32 to %08X\n", addr);
}

} // namespace twice
