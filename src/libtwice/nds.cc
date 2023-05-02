#include "libtwice/nds.h"
#include "libtwice/bus.h"
#include "libtwice/exception.h"
#include "libtwice/util.h"

using namespace twice;

NDS::NDS(u8 *arm7_bios, u8 *arm9_bios, u8 *firmware, u8 *cartridge,
		size_t cartridge_size)
	: arm7_bios(arm7_bios),
	  arm9_bios(arm9_bios),
	  firmware(firmware),
	  cartridge(cartridge),
	  cartridge_size(cartridge_size)
{
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
}
