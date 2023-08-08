#include "libtwice/machine.h"
#include "libtwice/exception.h"

#include "nds/nds.h"

namespace twice {

nds_machine::nds_machine(const nds_config& config)
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
	cartridge = file_map(pathname, MAX_CART_SIZE, file_map::MAP_MAX_SIZE);
}

void
nds_machine::boot(bool direct_boot)
{
	if (!cartridge) {
		throw twice_error("cartridge not loaded");
	}

	auto ctx = std::make_unique<nds_ctx>(arm7_bios.get_data(),
			arm9_bios.get_data(), firmware.get_data(),
			cartridge.get_data(), cartridge.get_size());
	if (direct_boot) {
		nds_direct_boot(ctx.get());
	} else {
		nds_firmware_boot(ctx.get());
	}
	nds = std::move(ctx);
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

void
nds_machine::update_touchscreen_state(int x, int y, bool down)
{
	if (!nds) return;

	if (down) {
		if (!(0 <= x && x <= 255)) return;
		if (!(0 <= y && y <= 191)) return;
	}

	if (down) {
		nds_set_touchscreen_state(nds.get(), x, y, down);
		nds->extkeyin &= ~BIT(6);
	} else {
		nds->extkeyin |= BIT(6);
	}
}

static bool
is_leap_year(int year)
{
	if (year % 400 == 0) {
		return true;
	}
	if (year % 100 == 0) {
		return false;
	}
	return year % 4 == 0;
}

void
nds_machine::update_real_time_clock(int year, int month, int day, int weekday,
		int hour, int minute, int second)
{
	if (!nds) return;

	if (!(2000 <= year && year <= 2099)) return;
	if (!(1 <= month && month <= 12)) return;
	if (!(1 <= day && day <= 31)) return;
	if (!(1 <= weekday && weekday <= 7)) return;
	if (!(0 <= hour && hour <= 23)) return;
	if (!(0 <= minute && minute <= 59)) return;
	if (!(0 <= second && second <= 59)) return;

	switch (month) {
	case 2:
		if (is_leap_year(year)) {
			if (day > 29) return;
		} else {
			if (day > 28) return;
		}
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		if (day > 30) return;
	}

	nds_set_rtc_time(nds.get(), year, month, day, weekday, hour, minute,
			second);
}

} // namespace twice
