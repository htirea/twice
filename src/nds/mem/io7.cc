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
	}

	LOG("nds 1 read 8 at %08X\n", addr);
	return 0;
}

u16
io7_read16(nds_ctx *nds, u32 addr)
{
	switch (addr) {
		IO_READ16_COMMON(1);
	case 0x4000136:
		return nds->extkeyin;
	case 0x40001C0:
		return nds->spicnt;
	case 0x40001C2:
		return spidata_read(nds);
		return 0;
	case 0x4000504:
		return nds->soundbias;
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
			nds->cpu[1]->halted = true;
			force_stop_cpu(nds->cpu[1]);
			break;
		case 3:
			nds->cpu[1]->stopped = true;
			force_stop_cpu(nds->cpu[1]);
			break;
		default:
			LOG("unhandled haltcnt\n");
		}
		nds->haltcnt = value;
		return;
	}

	LOG("nds 1 write 8 to %08X\n", addr);
}

void
io7_write16(nds_ctx *nds, u32 addr, u16 value)
{
	switch (addr) {
		IO_WRITE16_COMMON(1);
	case 0x40001C0:
		spicnt_write(nds, value);
		return;
	case 0x40001C2:
		spidata_write(nds, value);
		return;
	case 0x4000504:
		nds->soundbias = value & 0x3FF;
		return;
	}

	LOG("nds 1 write 16 to %08X\n", addr);
}

void
io7_write32(nds_ctx *nds, u32 addr, u32 value)
{
	switch (addr) {
		IO_WRITE32_COMMON(1);
	}

	if (0x4000400 <= addr && addr < 0x4000520) {
		/* TODO: sound registers */
		LOGV("[sound reg] nds 1 write 32 to %08X\n", addr);
		return;
	}

	LOG("nds 1 write 32 to %08X\n", addr);
}

} // namespace twice
