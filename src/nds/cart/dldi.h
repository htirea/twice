#ifndef TWICE_CART_DLDI_H
#define TWICE_CART_DLDI_H

#include "common/types.h"

#include "nds/cart/write_queue.h"

namespace twice {

struct nds_ctx;

struct dldi_controller {
	u8 *data{};
	size_t size{};
	u32 cmd{};
	u32 count{};
	u32 addr{};
	u32 write_start_addr{};
	u32 bytes_left{};
	u32 value_r{};
	bool error{};
	bool shutdown{};
	file_write_queue write_q;
};

void dldi_init(nds_ctx *nds);
void dldi_patch_cart(nds_ctx *nds);
u32 dldi_reg_read(nds_ctx *nds);
void dldi_reg_write(nds_ctx *nds, u32 value);
void sync_image_file(nds_ctx *nds, bool sync_whole_file);

} // namespace twice

#endif
