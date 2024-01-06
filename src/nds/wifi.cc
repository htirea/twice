#include "nds/wifi.h"

#include "common/logger.h"
#include "nds/nds.h"

namespace twice {

static u8 wifi_reg_read8(nds_ctx *, u32 offset);
static u16 wifi_reg_read16(nds_ctx *, u32 offset);
static u32 wifi_reg_read32(nds_ctx *, u32 offset);
static void wifi_reg_write16(nds_ctx *, u32 offset, u16 value);
static void wifi_reg_write32(nds_ctx *, u32 offset, u32 value);
static bool bb_reg_is_rw(u8 idx);

template <typename T>
T
io_wifi_read(nds_ctx *nds, u32 offset)
{
	switch (offset >> 12) {
	case 0:
	case 1:
	case 6:
	case 7:
		if constexpr (sizeof(T) == 1) {
			return wifi_reg_read8(nds, offset & 0xFFF);
		} else if constexpr (sizeof(T) == 2) {
			return wifi_reg_read16(nds, offset & 0xFFF);
		} else {
			return wifi_reg_read32(nds, offset & 0xFFF);
		}
	case 4:
	case 5:
		return readarr<T>(nds->wf.ram, offset & WIFI_RAM_MASK);
	default:
		return -1;
	}
}

template <typename T>
void
io_wifi_write(nds_ctx *nds, u32 offset, T value)
{
	if constexpr (sizeof(T) == 1) {
		return;
	}

	switch (offset >> 12) {
	case 0:
	case 1:
	case 6:
	case 7:
		if constexpr (sizeof(T) == 2) {
			wifi_reg_write16(nds, offset & 0xFFF, value);
		} else {
			wifi_reg_write32(nds, offset & 0xFFF, value);
		}
		break;
	case 4:
	case 5:
		writearr<T>(nds->wf.ram, offset & WIFI_RAM_MASK, value);
	}
}

static u8
wifi_reg_read8(nds_ctx *nds, u32 offset)
{
	auto& wf = nds->wf;

	switch (offset) {
	case 0x03C:
		return wf.powerstate;
	case 0x03D:
		return wf.powerstate >> 8;
	case 0x15C:
		return wf.bb_read;
	case 0x15D:
		return 0;
	case 0x15E:
		return 0;
	case 0x15F:
		return 0;
	case 0x160:
		return wf.bb_mode;
	case 0x161:
		return wf.bb_mode >> 8;
	case 0x168:
		return wf.bb_power;
	case 0x169:
		return wf.bb_power >> 8;
	}

	LOGV("wifi reg read 8 from %04X\n", offset);
	return readarr<u8>(nds->wf.wifi_regs, offset & WIFI_IO_REG_MASK);
}

static u16
wifi_reg_read16(nds_ctx *nds, u32 offset)
{
	auto& wf = nds->wf;

	switch (offset) {
	case 0x03C:
		return wf.powerstate;
	case 0x15C:
		return wf.bb_read;
	case 0x15E:
		return 0;
	case 0x160:
		return wf.bb_mode;
	case 0x168:
		return wf.bb_power;
	}

	LOGV("wifi reg read 16 from %04X\n", offset);
	return readarr<u16>(nds->wf.wifi_regs, offset & WIFI_IO_REG_MASK);
}

static u32
wifi_reg_read32(nds_ctx *nds, u32 offset)
{
	auto& wf = nds->wf;

	switch (offset) {
	case 0x03C:
		return wf.powerstate;
	}

	LOGV("wifi reg read 32 from %04X\n", offset);
	return readarr<u32>(nds->wf.wifi_regs, offset & WIFI_IO_REG_MASK);
}

static void
wifi_reg_write16(nds_ctx *nds, u32 offset, u16 value)
{
	auto& wf = nds->wf;

	switch (offset) {
	case 0x03C:
		wf.powerstate = (wf.powerstate & ~3) | (value & 3);
		return;
	case 0x158:
	{
		u8 idx = value & 0xFF;
		switch (value >> 12) {
		case 5:
			wf.bb_regs[idx] = wf.bb_write;
			break;
		case 6:
			switch (idx) {
			case 0x00:
				wf.bb_read = 0x6D;
				break;
			case 0x4D:
				wf.bb_read = 0x00;
				break;
			case 0x5D:
				wf.bb_read = 0x01;
				break;
			case 0x64:
				wf.bb_read = 0xFF;
				break;
			default:
				if (bb_reg_is_rw(idx)) {
					wf.bb_read = wf.bb_regs[idx];
				} else {
					wf.bb_read = 0x00;
				}
			}
		}
		return;
	}
	case 0x15A:
		wf.bb_write = value & 0xFF;
		return;
	case 0x160:
		wf.bb_mode = value & 0x4100;
		return;
	case 0x168:
		wf.bb_power = value & 0x800F;
		return;
	}

	LOGV("wifi reg write 16 to %04X\n", offset);
	writearr<u16>(wf.wifi_regs, offset & WIFI_IO_REG_MASK, value);
}

static void
wifi_reg_write32(nds_ctx *nds, u32 offset, u32 value)
{
	auto& wf = nds->wf;

	switch (offset) {
	case 0x03C:
		wf.powerstate = (wf.powerstate & ~3) | (value & 3);
		return;
	}

	LOGV("wifi reg write 32 to %04X\n", offset);
	writearr<u32>(wf.wifi_regs, offset & WIFI_IO_REG_MASK, value);
}

static bool
bb_reg_is_rw(u8 idx)
{
	return (0x01 <= idx && idx <= 0x0C) || (0x13 <= idx && idx <= 0x15) ||
	       (0x1B <= idx && idx <= 0x26) || (0x28 <= idx && idx <= 0x4C) ||
	       (0x4E <= idx && idx <= 0x5C) || (0x62 <= idx && idx <= 0x63) ||
	       idx == 0x65 || (0x67 <= idx && idx <= 0x68);
}

template u8 io_wifi_read<u8>(nds_ctx *, u32);
template u16 io_wifi_read<u16>(nds_ctx *, u32);
template u32 io_wifi_read<u32>(nds_ctx *, u32);
template void io_wifi_write<u8>(nds_ctx *, u32, u8);
template void io_wifi_write<u16>(nds_ctx *, u32, u16);
template void io_wifi_write<u32>(nds_ctx *, u32, u32);

} // namespace twice
