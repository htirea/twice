#include "libtwice/machine.h"
#include "libtwice/exception.h"

#include "nds/nds.h"

namespace twice {

Machine::Machine(Config& config)
	: config(config),
	  arm7_bios(config.data_dir + "bios7.bin", ARM7_BIOS_SIZE,
			  FileMap::MAP_EXACT_SIZE),
	  arm9_bios(config.data_dir + "bios9.bin", ARM9_BIOS_SIZE,
			  FileMap::MAP_EXACT_SIZE),
	  firmware(config.data_dir + "firmware.bin", FIRMWARE_SIZE,
			  FileMap::MAP_EXACT_SIZE)
{
}

Machine::~Machine() = default;

void
Machine::load_cartridge(const std::string& pathname)
{
	cartridge = { pathname, MAX_CART_SIZE, FileMap::MAP_MAX_SIZE };
}

void
Machine::direct_boot()
{
	if (!cartridge) {
		throw TwiceError("cartridge not loaded");
	}

	nds = std::make_unique<NDS>(arm7_bios.get_data(), arm9_bios.get_data(),
			firmware.get_data(), cartridge.get_data(),
			cartridge.get_size());

	nds->direct_boot();
}

void
Machine::run_frame()
{
	if (!nds) {
		return;
	}

	nds->run_frame();
}

void *
Machine::get_framebuffer()
{
	if (!nds) {
		return nullptr;
	}

	return nds->fb;
}

void
Machine::button_event(NdsButton button, bool down)
{
	if (!nds) {
		return;
	}

	using enum NdsButton;

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
