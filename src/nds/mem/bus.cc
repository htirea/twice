#include "nds/mem/bus.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/mem/io.h"
#include "nds/mem/vram.h"
#include "nds/nds.h"

namespace twice {

static const u8 gba_slot_nseq_timings[4] = { 10, 8, 6, 18 };
static const u8 gba_slot_seq_timings[2] = { 6, 4 };

static bool gpu_2d_memory_access_disabled(nds_ctx *nds, u32 addr);
template <typename T>
static T read_gba_rom_open_bus(u32 addr);
static void get_gba_slot_timings(u16 exmem, u8 *t);

template <typename T>
T
bus9_read(nds_ctx *nds, u32 addr)
{
	u8 *p = nds->bus9_read_pt[addr >> BUS9_PAGE_SHIFT];
	if (p) {
		return readarr<T>(p, addr & BUS9_PAGE_MASK);
	}

	return bus9_read_slow<T>(nds, addr);
}

template <typename T>
void
bus9_write(nds_ctx *nds, u32 addr, T value)
{
	u8 *p = nds->bus9_write_pt[addr >> BUS9_PAGE_SHIFT];
	if (p) {
		writearr<T>(p, addr & BUS9_PAGE_MASK, value);
		return;
	}

	bus9_write_slow<T>(nds, addr, value);
}

template <typename T>
T
bus7_read(nds_ctx *nds, u32 addr)
{
	u8 *p = nds->bus7_read_pt[addr >> BUS7_PAGE_SHIFT];
	if (p) {
		return readarr<T>(p, addr & BUS7_PAGE_MASK);
	}

	return bus7_read_slow<T>(nds, addr);
}

template <typename T>
void
bus7_write(nds_ctx *nds, u32 addr, T value)
{
	u8 *p = nds->bus7_write_pt[addr >> BUS7_PAGE_SHIFT];
	if (p) {
		writearr<T>(p, addr & BUS7_PAGE_MASK, value);
		return;
	}

	bus7_write_slow<T>(nds, addr, value);
}

template <typename T>
T
bus9_read_slow(nds_ctx *nds, u32 addr)
{
	bool gpu_disabled = gpu_2d_memory_access_disabled(nds, addr);

	switch (addr >> 24) {
	case 0x4:
		if constexpr (sizeof(T) == 1) {
			return io9_read8(nds, addr);
		} else if constexpr (sizeof(T) == 2) {
			return io9_read16(nds, addr);
		} else {
			return io9_read32(nds, addr);
		}
	case 0x5:
		if (gpu_disabled) {
			return 0;
		} else {
			return readarr<T>(nds->palette, addr & PALETTE_MASK);
		}
	case 0x6:
		return vram_read<T>(nds, addr);
	case 0x7:
		if (gpu_disabled) {
			return 0;
		} else {
			return readarr<T>(nds->oam, addr & OAM_MASK);
		}
	case 0x8:
	case 0x9:
		if (nds->gba_slot_cpu == 0) {
			return read_gba_rom_open_bus<T>(addr);
		} else {
			return 0;
		}
	case 0x2:
	case 0x3:
	case 0xFF:
		LOG("bus9 read: this region should be mapped: %02X\n",
				addr >> 24);
		return 0;
	default:
		LOG("undefined nds9 read at %08X\n", addr);
		return 0;
	}
}

template <typename T>
void
bus9_write_slow(nds_ctx *nds, u32 addr, T value)
{
	// TODO: check impact of putting everything except IO in fastmem

	bool gpu_disabled = gpu_2d_memory_access_disabled(nds, addr);

	switch (addr >> 24) {
	case 0x4:
		if constexpr (sizeof(T) == 1) {
			io9_write8(nds, addr, value);
		} else if constexpr (sizeof(T) == 2) {
			io9_write16(nds, addr, value);
		} else {
			io9_write32(nds, addr, value);
		}
		break;
	case 0x5:
		if (sizeof(T) != 1 && !gpu_disabled) {
			writearr<T>(nds->palette, addr & PALETTE_MASK, value);
		}
		break;
	case 0x6:
		if (sizeof(T) != 1) {
			vram_write<T>(nds, addr, value);
		}
		break;
	case 0x7:
		if (sizeof(T) != 1 && !gpu_disabled) {
			writearr<T>(nds->oam, addr & OAM_MASK, value);
		}
		break;
	case 0x2:
	case 0x3:
		LOG("bus9 write: this region should be mapped: %02X\n",
				addr >> 24);
		break;
	}
}

template <typename T>
T
bus7_read_slow(nds_ctx *nds, u32 addr)
{
	switch (addr >> 23) {
	case 0x0:
		if (addr < ARM7_BIOS_SIZE) {
			if (nds->cpu[1]->gpr[15] >= ARM7_BIOS_SIZE) {
				return -1;
			} else {
				return readarr<T>(nds->arm7_bios, addr);
			}
		} else {
			return 0;
		}
	case 0x40 >> 3:
		if constexpr (sizeof(T) == 1) {
			return io7_read8(nds, addr);
		} else if constexpr (sizeof(T) == 2) {
			return io7_read16(nds, addr);
		} else {
			return io7_read32(nds, addr);
		}
	case 0x48 >> 3:
		if (addr < 0x4810000) {
			return io_wifi_read<T>(nds, addr & WIFI_REGION_MASK);
		} else {
			return 0;
		}
		break;
	case 0x60 >> 3:
	case 0x68 >> 3:
		return vram_arm7_read<T>(nds, addr);
		break;
	case 0x80 >> 3:
	case 0x88 >> 3:
	case 0x90 >> 3:
	case 0x98 >> 3:
		if (nds->gba_slot_cpu == 1) {
			return read_gba_rom_open_bus<T>(addr);
		} else {
			return 0;
		}
	case 0x20 >> 3:
	case 0x28 >> 3:
	case 0x30 >> 3:
	case 0x38 >> 3:
		LOG("bus7 read: this region should be mapped: %02X\n",
				addr >> 24 << 3);
		return 0;
	default:
		return 0;
	}
}

template <typename T>
void
bus7_write_slow(nds_ctx *nds, u32 addr, T value)
{
	switch (addr >> 23) {
	case 0x40 >> 3:
		if constexpr (sizeof(T) == 1) {
			io7_write8(nds, addr, value);
		} else if constexpr (sizeof(T) == 2) {
			io7_write16(nds, addr, value);
		} else {
			io7_write32(nds, addr, value);
		}
		break;
	case 0x48 >> 3:
		if (addr < 0x4810000) {
			io_wifi_write<T>(nds, addr & WIFI_REGION_MASK, value);
		}
		break;
	case 0x60 >> 3:
	case 0x68 >> 3:
		vram_arm7_write<T>(nds, addr, value);
		break;
	case 0x20 >> 3:
	case 0x28 >> 3:
	case 0x30 >> 3:
	case 0x38 >> 3:
		LOG("bus7 write: this region should be mapped: %02X\n",
				addr >> 24 << 3);
		break;
	}
}

void
bus_tables_init(nds_ctx *nds)
{
	arm7_init_tables(nds->arm7.get());
	reset_page_tables(nds);
	reset_timing_tables(nds);
}

void
reset_page_tables(nds_ctx *nds)
{
	update_bus9_page_tables(nds, 0, 4_GiB);
	update_bus7_page_tables(nds, 0, 4_GiB);
}

void
reset_timing_tables(nds_ctx *nds)
{
	update_bus9_timing_tables(nds, 0, 4_GiB);
	update_bus7_timing_tables(nds, 0, 4_GiB);
}

void
update_bus9_page_tables(nds_ctx *nds, u64 start, u64 end)
{
	auto& pt_r = nds->bus9_read_pt;
	auto& pt_w = nds->bus9_write_pt;

	for (u64 addr = start; addr < end; addr += BUS9_PAGE_SIZE) {
		u32 page = addr >> BUS9_PAGE_SHIFT;

		switch (addr >> 24) {
		case 0x2:
			pt_r[page] = &nds->main_ram[addr & MAIN_RAM_MASK];
			pt_w[page] = pt_r[page];
			break;
		case 0x3:
		{
			u8 *p = nds->shared_wram_p[0];
			pt_r[page] = &p[addr & nds->shared_wram_mask[0]];
			pt_w[page] = pt_r[page];
			break;
		}
		case 0xFF:
			if (addr < 0xFFFF0000) {
				pt_r[page] = nullptr;
			} else if (addr < 0xFFFF0000 + ARM9_BIOS_SIZE) {
				pt_r[page] = &nds->arm9_bios[addr &
							     ARM9_BIOS_MASK];
			} else {
				pt_r[page] = nds->zero_page;
			}
			pt_w[page] = nullptr;
			break;
		default:
			pt_r[page] = nullptr;
			pt_w[page] = nullptr;
		}
	}

	update_arm9_page_tables(nds->arm9.get(), start, end);
}

void
update_bus9_timing_tables(nds_ctx *nds, u64 start, u64 end)
{
	std::array<u8, 4> code_timing{};
	std::array<u8, 4> data_timing{};
	std::array<u8, 4> cpu_data_timing{};

	for (u64 addr = start; addr < end; addr += BUS_TIMING_SIZE) {
		u32 page = addr >> BUS_TIMING_SHIFT;
		bool def = false;

		switch (addr >> 24) {
		case 0x2: /* TODO: cache */
			code_timing = { 1 };
			data_timing = { 18, 4, 16, 2 };
			cpu_data_timing = { 1, 1, 1, 1 };
			break;
		case 0x5:
		case 0x6: /* TODO: VRAM unmapped timings? */
			code_timing = { 10 };
			data_timing = { 4, 4, 2, 2 };
			cpu_data_timing = { 10, 4, 8, 2 };
			break;
		case 0x8:
		case 0x9:
			if (nds->gba_slot_cpu == 0) {
				get_gba_slot_timings(nds->exmem[0],
						data_timing.data());
				code_timing = { (u8)(data_timing[0] + 3) };
				cpu_data_timing = data_timing;
				cpu_data_timing[0] += 3;
				cpu_data_timing[2] += 3;

				for (u32 i = 0; i < 4; i++) {
					code_timing[i] <<= 1;
					data_timing[i] <<= 1;
					cpu_data_timing[i] <<= 1;
				}
			} else {
				def = true;
			}
			break;
		case 0x3:
		case 0x4:
		case 0x7:
		case 0xFF:
		default:
			def = true;
		}

		if (def) {
			code_timing = { 8 };
			data_timing = { 2, 2, 2, 2 };
			cpu_data_timing = { 8, 2, 8, 2 };
		}

		nds->arm9_code_timings[page] = code_timing;
		nds->arm9_data_timings[page] = cpu_data_timing;
		nds->bus9_data_timings[page] = data_timing;
	}

	update_arm9_page_tables(nds->arm9.get(), start, end);
}

void
update_bus7_page_tables(nds_ctx *nds, u64 start, u64 end)
{
	auto& pt_r = nds->bus7_read_pt;
	auto& pt_w = nds->bus7_write_pt;

	for (u64 addr = start; addr < end; addr += BUS7_PAGE_SIZE) {
		u32 page = addr >> BUS7_PAGE_SHIFT;

		switch (addr >> 23) {
		case 0x20 >> 3:
		case 0x28 >> 3:
			pt_r[page] = &nds->main_ram[addr & MAIN_RAM_MASK];
			pt_w[page] = pt_r[page];
			break;
		case 0x30 >> 3:
		{
			u8 *p = nds->shared_wram_p[1];
			pt_r[page] = &p[addr & nds->shared_wram_mask[1]];
			pt_w[page] = pt_r[page];
			break;
		}
		case 0x38 >> 3:
			pt_r[page] = &nds->arm7_wram[addr & ARM7_WRAM_MASK];
			pt_w[page] = pt_r[page];
			break;
		default:
			pt_r[page] = nullptr;
			pt_w[page] = nullptr;
		}
	}
}

void
update_bus7_timing_tables(nds_ctx *nds, u64 start, u64 end)
{
	std::array<u8, 4> code_timing{};
	std::array<u8, 4> data_timing{};
	std::array<u8, 4> cpu_data_timing{};

	for (u64 addr = start; addr < end; addr += BUS_TIMING_SIZE) {
		u32 page = addr >> BUS_TIMING_SHIFT;
		bool def = false;

		switch (addr >> 24) {
		case 0x2:
			code_timing = { 9, 2, 8, 1 };
			data_timing = { 9, 2, 8, 1 };
			cpu_data_timing = { 10, 2, 9, 1 };
			break;
		case 0x5:
		case 0x6:
			code_timing = { 2, 2, 1, 1 };
			data_timing = { 2, 2, 1, 1 };
			cpu_data_timing = { 1, 2, 1, 1 }; /* TODO: check */
			break;
		case 0x8:
		case 0x9:
			if (nds->gba_slot_cpu == 1) {
				get_gba_slot_timings(nds->exmem[1],
						data_timing.data());
				code_timing = data_timing;
				cpu_data_timing = data_timing;
				cpu_data_timing[0] -= 1;
				cpu_data_timing[2] -= 1;
			} else {
				def = true;
			}
			break;
		default:
			def = true;
		}

		if (def) {
			code_timing = { 1, 1, 1, 1 };
			data_timing = { 1, 1, 1, 1 };
			cpu_data_timing = { 1, 1, 1, 1 };
		}

		nds->arm7_code_timings[page] = code_timing;
		nds->arm7_data_timings[page] = cpu_data_timing;
		nds->arm7_code_timings[page] = data_timing;
	}
}

static bool
gpu_2d_memory_access_disabled(nds_ctx *nds, u32 addr)
{
	return (!nds->gpu2d[0].enabled && !(addr & 0x400)) ||
	       (!nds->gpu2d[1].enabled && (addr & 0x400));
}

template <typename T>
static T
read_gba_rom_open_bus(u32 addr)
{
	if constexpr (sizeof(T) == 1) {
		return (addr >> 1) >> 8 * (addr & 1);
	} else if constexpr (sizeof(T) == 2) {
		return addr >> 1;
	} else {
		u16 lo = addr >> 1;
		return (u32)(lo + 1) << 16 | lo;
	}
}

static void
get_gba_slot_timings(u16 exmem, u8 *t)
{
	u8 nseq16 = gba_slot_nseq_timings[exmem >> 2 & 3];
	u8 seq16 = gba_slot_seq_timings[exmem >> 4 & 1];
	t[0] = nseq16 + seq16;
	t[1] = seq16 + seq16;
	t[2] = nseq16;
	t[3] = seq16;
}

template u8 bus9_read<u8>(nds_ctx *nds, u32 addr);
template u16 bus9_read<u16>(nds_ctx *nds, u32 addr);
template u32 bus9_read<u32>(nds_ctx *nds, u32 addr);
template void bus9_write<u8>(nds_ctx *nds, u32 addr, u8 value);
template void bus9_write<u16>(nds_ctx *nds, u32 addr, u16 value);
template void bus9_write<u32>(nds_ctx *nds, u32 addr, u32 value);

template u8 bus7_read<u8>(nds_ctx *nds, u32 addr);
template u16 bus7_read<u16>(nds_ctx *nds, u32 addr);
template u32 bus7_read<u32>(nds_ctx *nds, u32 addr);
template void bus7_write<u8>(nds_ctx *nds, u32 addr, u8 value);
template void bus7_write<u16>(nds_ctx *nds, u32 addr, u16 value);
template void bus7_write<u32>(nds_ctx *nds, u32 addr, u32 value);

template u8 bus9_read_slow<u8>(nds_ctx *nds, u32 addr);
template u16 bus9_read_slow<u16>(nds_ctx *nds, u32 addr);
template u32 bus9_read_slow<u32>(nds_ctx *nds, u32 addr);
template void bus9_write_slow<u8>(nds_ctx *nds, u32 addr, u8 value);
template void bus9_write_slow<u16>(nds_ctx *nds, u32 addr, u16 value);
template void bus9_write_slow<u32>(nds_ctx *nds, u32 addr, u32 value);

template u8 bus7_read_slow<u8>(nds_ctx *nds, u32 addr);
template u16 bus7_read_slow<u16>(nds_ctx *nds, u32 addr);
template u32 bus7_read_slow<u32>(nds_ctx *nds, u32 addr);
template void bus7_write_slow<u8>(nds_ctx *nds, u32 addr, u8 value);
template void bus7_write_slow<u16>(nds_ctx *nds, u32 addr, u16 value);
template void bus7_write_slow<u32>(nds_ctx *nds, u32 addr, u32 value);

} // namespace twice
