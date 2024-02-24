#ifndef TWICE_CART_BACKUP_H
#define TWICE_CART_BACKUP_H

#include "common/types.h"

namespace twice {

struct nds_ctx;

struct cartridge_backup {
	u8 *data{};
	size_t size{};
	int savetype{};
	u8 stat_reg{};
	u32 jedec_id{};
	bool cs_active{};

	u8 command{};
	u32 count{};
	u32 addr{};
	u8 ir_command{};
	u32 ir_count{};
	u32 write_start_addr{};
	bool write_in_progress{};
	std::optional<std::pair<u32, u32>> dirty_interval;
};

void cartridge_backup_init(nds_ctx *nds, int savetype);
void auxspicnt_write_l(nds_ctx *nds, int cpuid, u8 value);
void auxspicnt_write_h(nds_ctx *nds, int cpuid, u8 value);
void auxspicnt_write(nds_ctx *nds, int cpuid, u16 value);
void auxspidata_write(nds_ctx *nds, int cpuid, u8 value);
void event_auxspi_transfer_complete(nds_ctx *nds, intptr_t, timestamp);
void write_whole_savefile(nds_ctx *nds);

} // namespace twice

#endif
