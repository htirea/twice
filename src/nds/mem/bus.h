#ifndef TWICE_MEM_BUS_H
#define TWICE_MEM_BUS_H

#include "common/types.h"

namespace twice {

enum : u32 {
	BUS9_PAGE_SHIFT = 12,
	BUS9_PAGE_SIZE = (u32)1 << BUS9_PAGE_SHIFT,
	BUS9_PAGE_MASK = BUS9_PAGE_SIZE - 1,
	BUS9_PAGE_TABLE_SIZE = (u32)1 << (32 - BUS9_PAGE_SHIFT),

	BUS7_PAGE_SHIFT = 14,
	BUS7_PAGE_SIZE = (u32)1 << BUS7_PAGE_SHIFT,
	BUS7_PAGE_MASK = BUS7_PAGE_SIZE - 1,
	BUS7_PAGE_TABLE_SIZE = (u32)1 << (32 - BUS7_PAGE_SHIFT),

	BUS_TIMING_SHIFT = 24,
	BUS_TIMING_SIZE = (u32)1 << BUS_TIMING_SHIFT,
	BUS_TIMING_MASK = BUS_TIMING_SIZE - 1,
	BUS_TIMING_TABLE_SIZE = (u32)1 << (32 - BUS_TIMING_SHIFT),
};

struct nds_ctx;

template <typename T>
T bus9_read(nds_ctx *nds, u32 addr);
template <typename T>
void bus9_write(nds_ctx *nds, u32 addr, T value);
template <typename T>
T bus7_read(nds_ctx *nds, u32 addr);
template <typename T>
void bus7_write(nds_ctx *nds, u32 addr, T value);

template <typename T>
T bus9_read_slow(nds_ctx *nds, u32 addr);
template <typename T>
void bus9_write_slow(nds_ctx *nds, u32 addr, T value);
template <typename T>
T bus7_read_slow(nds_ctx *nds, u32 addr);
template <typename T>
void bus7_write_slow(nds_ctx *nds, u32 addr, T value);

template <int cpuid, typename T>
T
bus_read(nds_ctx *nds, u32 addr)
{
	if constexpr (cpuid == 0) {
		return bus9_read<T>(nds, addr);
	} else {
		return bus7_read<T>(nds, addr);
	}
}

template <int cpuid, typename T>
void
bus_write(nds_ctx *nds, u32 addr, T value)
{
	if constexpr (cpuid == 0) {
		bus9_write<T>(nds, addr, value);
	} else {
		bus7_write<T>(nds, addr, value);
	}
}

void bus_tables_init(nds_ctx *nds);
void reset_page_tables(nds_ctx *nds);
void reset_timing_tables(nds_ctx *nds);
void update_bus9_page_tables(nds_ctx *nds, u64 start, u64 end);
void update_bus9_timing_tables(nds_ctx *nds, u64 start, u64 end);
void update_bus7_page_tables(nds_ctx *nds, u64 start, u64 end);
void update_bus7_timing_tables(nds_ctx *nds, u64 start, u64 end);

} // namespace twice

#endif
