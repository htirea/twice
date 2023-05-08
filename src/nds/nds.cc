#include "nds/nds.h"
#include "nds/arm/arm.h"
#include "nds/arm/arm7.h"
#include "nds/arm/arm9.h"
#include "nds/mem/bus.h"
#include "nds/mem/io.h"

#include "common/util.h"

#include "libtwice/exception.h"

namespace twice {

NDS::NDS(u8 *arm7_bios, u8 *arm9_bios, u8 *firmware, u8 *cartridge,
		size_t cartridge_size)
	: arm9(std::make_unique<Arm9>(this)),
	  arm7(std::make_unique<Arm7>(this)),
	  arm7_bios(arm7_bios),
	  arm9_bios(arm9_bios),
	  firmware(firmware),
	  cartridge(cartridge),
	  cartridge_size(cartridge_size)
{
	cpu[0] = arm9.get();
	cpu[1] = arm7.get();
	wramcnt_write(this, 0x0);
}

NDS::~NDS() = default;

static void
parse_header(NDS *nds, u32 *entry_addr_ret)
{
	u32 rom_offset[2]{ readarr<u32>(nds->cartridge, 0x20),
		readarr<u32>(nds->cartridge, 0x30) };
	u32 entry_addr[2]{ readarr<u32>(nds->cartridge, 0x24),
		readarr<u32>(nds->cartridge, 0x34) };
	u32 ram_addr[2]{ readarr<u32>(nds->cartridge, 0x28),
		readarr<u32>(nds->cartridge, 0x38) };
	u32 transfer_size[2]{ readarr<u32>(nds->cartridge, 0x2C),
		readarr<u32>(nds->cartridge, 0x3C) };

	if (rom_offset[0] >= nds->cartridge_size) {
		throw TwiceError("arm9 rom offset too large");
	}

	if (rom_offset[0] + transfer_size[0] > nds->cartridge_size) {
		throw TwiceError("arm9 transfer size too large");
	}

	if (rom_offset[1] >= nds->cartridge_size) {
		throw TwiceError("arm7 rom offset too large");
	}

	if (rom_offset[1] + transfer_size[1] > nds->cartridge_size) {
		throw TwiceError("arm7 transfer size too large");
	}

	if ((rom_offset[0] | rom_offset[1]) & 0x3) {
		throw TwiceError("cartridge rom offset unaligned");
	}

	if ((entry_addr[0] | entry_addr[1]) & 0x3) {
		throw TwiceError("cartridge entry addr unaligned");
	}

	if ((ram_addr[0] | ram_addr[1]) & 0x3) {
		throw TwiceError("cartridge ram addr unaligned");
	}

	if ((transfer_size[0] | transfer_size[1]) & 0x3) {
		throw TwiceError("cartridge transfer size unaligned");
	}

	for (u32 i = 0; i < transfer_size[0]; i += 4) {
		u32 value = readarr<u32>(nds->cartridge, rom_offset[0] + i);
		bus9_write<u32>(nds, ram_addr[0] + i, value);
	}

	for (u32 i = 0; i < transfer_size[1]; i += 4) {
		u32 value = readarr<u32>(nds->cartridge, rom_offset[1] + i);
		bus7_write<u32>(nds, ram_addr[1] + i, value);
	}

	entry_addr_ret[0] = entry_addr[0];
	entry_addr_ret[1] = entry_addr[1];

	fprintf(stderr, "entry addr: %08X %08X\n", entry_addr[0],
			entry_addr[1]);
}

void
NDS::direct_boot()
{
	u32 entry_addr[2];

	parse_header(this, entry_addr);

	arm9->cp15_write(0x100, 0x00012078);
	arm9->cp15_write(0x910, 0x3000000A);
	arm9->cp15_write(0x911, 0x00000020);

	/* TODO: remove this later */
	arm9->cp15_write(0x910, 0x0080000A);

	arm9->gpr[12] = entry_addr[0];
	arm9->gpr[13] = 0x03002F7C;
	arm9->gpr[14] = entry_addr[0];
	arm9->bankedr[MODE_IRQ][0] = 0x03003F80;
	arm9->bankedr[MODE_SVC][0] = 0x03003FC0;
	arm9->jump(entry_addr[0]);

	arm7->gpr[12] = entry_addr[1];
	arm7->gpr[13] = 0x0380FD80;
	arm7->gpr[14] = entry_addr[1];
	arm7->bankedr[MODE_IRQ][0] = 0x0380FF80;
	arm7->bankedr[MODE_SVC][0] = 0x0380FFC0;
	arm7->jump(entry_addr[1]);

	/* TODO: more stuff for direct booting */
}

u32
ABGR1555_TO_ABGR8888(u16 color)
{
	u8 r = color & 0x1F;
	u8 g = color >> 5 & 0x1F;
	u8 b = color >> 10 & 0x1F;
	u8 a = color >> 15;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	a = a * 0xFF;

	return (a << 24) | (b << 16) | (g << 8) | r;
}

void
NDS::run_frame()
{
	cycles = 0;

	while (cycles < 408960) {
		arm9->step();
		arm9->step();
		arm7->step();
	}

	dispstat[0] |= 1;
	dispstat[1] |= 1;

	for (u32 i = 0; i < NDS_FB_SZ; i++) {
		fb[i] = ABGR1555_TO_ABGR8888(readarr<u16>(vram, i * 2));
	}

	while (cycles < 558060) {
		arm9->step();
		arm9->step();
		arm7->step();
	}

	dispstat[0] &= ~1;
	dispstat[1] &= ~1;

	while (cycles < 560190) {
		arm9->step();
		arm9->step();
		arm7->step();
	}
}

} // namespace twice
