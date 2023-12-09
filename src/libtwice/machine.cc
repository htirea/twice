#include "libtwice/nds/machine.h"
#include "libtwice/exception.h"

#include "nds/nds.h"

#include <filesystem>

namespace twice {

nds_machine::nds_machine(const nds_config& config)
	: config(config),
	  arm7_bios(config.data_dir + "bios7.bin", ARM7_BIOS_SIZE,
			  file_map::FILEMAP_PRIVATE | file_map::FILEMAP_EXACT),
	  arm9_bios(config.data_dir + "bios9.bin", ARM9_BIOS_SIZE,
			  file_map::FILEMAP_PRIVATE | file_map::FILEMAP_EXACT),
	  firmware(config.data_dir + "firmware.bin", FIRMWARE_SIZE,
			  file_map::FILEMAP_PRIVATE | file_map::FILEMAP_EXACT)
{
}

nds_machine::~nds_machine() = default;

void
nds_machine::load_cartridge(const std::string& pathname)
{
	nds_save_info save_info{ SAVETYPE_UNKNOWN, 0 };

	load_cartridge(pathname, save_info);
}

void
nds_machine::load_cartridge(
		const std::string& pathname, nds_save_info save_info)
{
	namespace fs = std::filesystem;

	auto cartridge = file_map(pathname, MAX_CART_SIZE,
			file_map::FILEMAP_PRIVATE | file_map::FILEMAP_LIMIT);
	if (cartridge.size() < 0x160) {
		throw twice_error("cartridge size too small: " + pathname);
	}

	if (save_info.type == SAVETYPE_UNKNOWN) {
		save_info = nds_get_save_info(cartridge);
	}

	file_map savefile;
	if (save_info.type != SAVETYPE_NONE) {
		std::string savepath =
				fs::path(pathname).replace_extension(".sav");
		savefile = file_map(savepath, save_info.size,
				file_map::FILEMAP_SHARED);
	}

	this->cartridge = std::move(cartridge);
	this->savefile = std::move(savefile);
	this->save_info = save_info;
}

void
nds_machine::boot(bool direct_boot)
{
	if (direct_boot && !cartridge) {
		throw twice_error("cartridge not loaded");
	}

	if (cartridge && save_info.type == SAVETYPE_UNKNOWN) {
		throw twice_error("unknown save type");
	}

	auto ctx = std::make_unique<nds_ctx>(arm7_bios.data(),
			arm9_bios.data(), firmware.data(), cartridge.data(),
			cartridge.size(), savefile.data(), savefile.size(),
			save_info.type);
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
	if (!nds)
		return;

	nds_run_frame(nds.get());
}

bool
nds_machine::is_shutdown()
{
	if (!nds)
		return true;

	return nds->shutdown;
}

u32 *
nds_machine::get_framebuffer()
{
	if (!nds)
		return nullptr;

	return nds->fb;
}

s16 *
nds_machine::get_audio_buffer()
{
	if (!nds)
		return nullptr;

	return nds->audio_buf;
}

u32
nds_machine::get_audio_buffer_size()
{
	if (!nds)
		return 0;

	return nds->last_audio_buf_size;
}

void
nds_machine::button_event(nds_button button, bool down)
{
	if (!nds)
		return;

	using enum nds_button;

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
	if (!nds)
		return;

	if (down) {
		x = std::clamp(x, 0, 255);
		y = std::clamp(y, 0, 191);
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
	if (year % 400 == 0)
		return true;

	if (year % 100 == 0)
		return false;

	return year % 4 == 0;
}

void
nds_machine::update_real_time_clock(int year, int month, int day, int weekday,
		int hour, int minute, int second)
{
	if (!nds)
		return;

	if (!(2000 <= year && year <= 2099))
		return;
	if (!(1 <= month && month <= 12))
		return;
	if (!(1 <= day && day <= 31))
		return;
	if (!(1 <= weekday && weekday <= 7))
		return;
	if (!(0 <= hour && hour <= 23))
		return;
	if (!(0 <= minute && minute <= 59))
		return;
	if (!(0 <= second && second <= 59))
		return;

	switch (month) {
	case 2:
		if (is_leap_year(year)) {
			if (day > 29)
				return;
		} else {
			if (day > 28)
				return;
		}
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		if (day > 30)
			return;
	}

	nds_set_rtc_time(nds.get(), year, month, day, weekday, hour, minute,
			second);
}

std::pair<double, double>
nds_machine::get_cpu_usage()
{
	if (!nds)
		return { 0, 0 };

	return { nds->arm9_usage, nds->arm7_usage };
}

void
nds_machine::dump_profiler_report()
{
	if (!nds)
		return;

	nds_dump_prof(nds.get());
}

} // namespace twice
