#include "libtwice/nds.h"

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

void
NDS::direct_boot()
{
}
