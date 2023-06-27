#include "libtwice/machine.h"
#include "libtwice/exception.h"

#include "nds/nds.h"

namespace twice {

nds_machine::nds_machine(nds_config& config)
	: config(config),
	  arm7_bios(config.data_dir + "bios7.bin", ARM7_BIOS_SIZE,
			  file_map::MAP_EXACT_SIZE),
	  arm9_bios(config.data_dir + "bios9.bin", ARM9_BIOS_SIZE,
			  file_map::MAP_EXACT_SIZE),
	  firmware(config.data_dir + "firmware.bin", FIRMWARE_SIZE,
			  file_map::MAP_EXACT_SIZE)
{
}

nds_machine::~nds_machine() = default;

void
nds_machine::load_cartridge(const std::string& pathname)
{
	cartridge = { pathname, MAX_CART_SIZE, file_map::MAP_MAX_SIZE };
}

void
nds_machine::direct_boot()
{
	if (!cartridge) {
		throw twice_error("cartridge not loaded");
	}

	nds = std::make_unique<nds_ctx>(arm7_bios.get_data(),
			arm9_bios.get_data(), firmware.get_data(),
			cartridge.get_data(), cartridge.get_size());

	nds_direct_boot(nds.get());
}

void
nds_machine::run_frame()
{
	if (!nds) {
		return;
	}

	nds_run_frame(nds.get());
}

void *
nds_machine::get_framebuffer()
{
	if (!nds) {
		return nullptr;
	}

	return nds->fb;
}

void
nds_machine::button_event(nds_button button, bool down)
{
	if (!nds) {
		return;
	}

	using enum nds_button;

	if (button == NONE) {
		return;
	}

	switch (button) {
	case A:
	case B:
	case SELECT:
	case START:
	case RIGHT:
	case LEFT:
	case UP:
	case DOWN:
	case R:
	case L:
		if (down) {
			nds->keyinput &= ~BIT((int)button);
		} else {
			nds->keyinput |= BIT((int)button);
		}
		break;
	case X:
		if (down) {
			nds->extkeyin &= ~BIT(0);
		} else {
			nds->extkeyin |= BIT(0);
		}
		break;
	case Y:
		if (down) {
			nds->extkeyin &= ~BIT(1);
		} else {
			nds->extkeyin |= BIT(1);
		}
		break;
	default:;
	}
}

} // namespace twice
