#include "libtwice/nds/machine.h"
#include "libtwice/exception.h"

#include "common/date.h"
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
	auto cartridge = file_map(pathname, MAX_CART_SIZE,
			file_map::FILEMAP_PRIVATE | file_map::FILEMAP_LIMIT);
	if (cartridge.size() < 0x160) {
		throw twice_error("cartridge size too small: " + pathname);
	}

	this->cartridge = std::move(cartridge);
}

void
nds_machine::set_savetype(nds_savetype savetype)
{
	this->savetype = savetype;
}

void
nds_machine::load_savefile(const std::string& pathname)
{
	if (savetype == SAVETYPE_NONE)
		return;

	if (savetype == SAVETYPE_UNKNOWN) {
		try {
			auto savefile = file_map(pathname, 0,
					file_map::FILEMAP_PRIVATE);
			auto size = savefile.size();
			savetype = nds_savetype_from_size(size);
		} catch (const file_map::file_map_error& e) {
		}
	}

	if (savetype == SAVETYPE_UNKNOWN)
		return;

	auto savefile = file_map(pathname, nds_savetype_to_size(savetype),
			file_map::FILEMAP_SHARED | file_map::FILEMAP_EXACT);
	this->savefile = std::move(savefile);
}

void
nds_machine::boot(bool direct_boot)
{
	if (direct_boot && !cartridge) {
		throw twice_error("cartridge not loaded");
	}

	if (cartridge && savetype == SAVETYPE_UNKNOWN) {
		savetype = nds_get_save_info(cartridge).type;
	}

	if (cartridge && !savefile && savetype != SAVETYPE_NONE) {
		std::string savepath =
				std::filesystem::path(cartridge.get_pathname())
						.replace_extension(".sav")
						.string();
		load_savefile(savepath);
	}

	if (cartridge && savetype == SAVETYPE_UNKNOWN) {
		throw twice_error("unknown savetype");
	}

	if (cartridge) {
		cartridge.remap();
	}

	auto ctx = create_nds_ctx(arm7_bios.data(), arm9_bios.data(),
			firmware.data(), cartridge.data(), cartridge.size(),
			savefile.data(), savefile.size(), savetype);
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

	x = std::clamp(x, 0, 255);
	y = std::clamp(y, 0, 191);
	nds_set_touchscreen_state(nds.get(), x, y, down);
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

	if (day > get_days_in_month(month, year))
		return;

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
nds_machine::set_use_16_bit_audio(bool use_16_bit_audio)
{
	nds->use_16_bit_audio = use_16_bit_audio;
}

void
nds_machine::dump_profiler_report()
{
	if (!nds)
		return;

	nds_dump_prof(nds.get());
}

} // namespace twice
