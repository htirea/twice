#include "libtwice/nds/machine.h"
#include "libtwice/exception.h"

#include "common/date.h"
#include "nds/nds.h"

#include <filesystem>

namespace twice {

nds_machine::nds_machine(const nds_config& config) : config(config)
{
	try {
		auto pathname = config.arm9_bios_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "bios9.bin";
		}
		load_system_file(pathname, nds_system_file::ARM9_BIOS);
	} catch (const file_map::file_map_error& e) {
	}

	try {
		auto pathname = config.arm7_bios_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "bios7.bin";
		}
		load_system_file(pathname, nds_system_file::ARM7_BIOS);
	} catch (const file_map::file_map_error& e) {
	}

	try {
		auto pathname = config.firmware_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "firmware.bin";
		}
		load_system_file(pathname, nds_system_file::FIRMWARE);
	} catch (const file_map::file_map_error& e) {
	}
}

nds_machine::~nds_machine() = default;

void
nds_machine::load_system_file(
		const std::filesystem::path& pathname, nds_system_file type)
{
	if (nds) {
		/* TODO: handle loading files while running */
		throw twice_error("The system file cannot be loaded while the "
				  "machine is running.");
	}

	int flags = file_map::FILEMAP_PRIVATE | file_map::FILEMAP_EXACT;

	switch (type) {
	case nds_system_file::ARM9_BIOS:
		arm9_bios = file_map(pathname, ARM9_BIOS_SIZE, flags);
		break;
	case nds_system_file::ARM7_BIOS:
		arm7_bios = file_map(pathname, ARM7_BIOS_SIZE, flags);
		break;
	case nds_system_file::FIRMWARE:
		firmware = file_map(pathname, FIRMWARE_SIZE, flags);
		break;
	}
}

void
nds_machine::load_cartridge(const std::filesystem::path& pathname)
{
	if (nds) {
		/* TODO: handle loading cartridge while running */
		throw twice_error("The cartridge cannot be loaded while the "
				  "machine is running.");
	}

	auto cartridge = file_map(pathname, MAX_CART_SIZE,
			file_map::FILEMAP_PRIVATE | file_map::FILEMAP_LIMIT);
	if (cartridge.size() < 0x160) {
		throw twice_error("The cartridge size is too small.");
	}

	this->cartridge = std::move(cartridge);
	savefile = file_map();
	savetype = SAVETYPE_UNKNOWN;
}

void
nds_machine::eject_cartridge()
{
	if (nds) {
		/* TODO: handle ejecting cartridge while running */
		throw twice_error("The cartridge cannot be ejected while the "
				  "machine is running.");
	}

	cartridge = file_map();
	savefile = file_map();
	savetype = SAVETYPE_UNKNOWN;
}

void
nds_machine::set_savetype(nds_savetype savetype)
{
	if (nds) {
		/* TODO: handle changing save type while running */
		throw twice_error("The save type cannot be changed while the "
				  "machine is running.");
	}

	this->savetype = savetype;
}

void
nds_machine::load_savefile(const std::filesystem::path& pathname)
{
	int flags = file_map::FILEMAP_SHARED | file_map::FILEMAP_MUST_EXIST;
	auto savefile = file_map(pathname, 0, flags);

	if (savetype == SAVETYPE_UNKNOWN) {
		savetype = nds_savetype_from_size(savefile.size());
	}

	this->savefile = std::move(savefile);
}

void
nds_machine::load_or_create_savefile(const std::filesystem::path& pathname,
		nds_savetype try_savetype)
{
	try {
		load_savefile(pathname);
	} catch (const file_map::file_map_error& e) {
		if (try_savetype == SAVETYPE_UNKNOWN) {
			throw twice_error("The save type must be set.");
		}

		savefile = file_map(pathname,
				nds_savetype_to_size(try_savetype),
				file_map::FILEMAP_SHARED);
		savetype = try_savetype;
	}
}

void
nds_machine::boot(bool direct_boot)
{
	if (nds)
		return;

	if (!arm9_bios || !arm7_bios || !firmware) {
		throw twice_error("The NDS system files must be loaded.");
	}

	if (direct_boot && !cartridge) {
		throw twice_error("Cartridge must be loaded for direct "
				  "boot.");
	}

	if (cartridge) {
		cartridge.remap();
	}

	if (cartridge && savetype != SAVETYPE_NONE && !savefile) {
		auto pathname = cartridge.get_pathname().replace_extension(
				".sav");
		nds_savetype try_savetype = savetype;
		if (try_savetype == SAVETYPE_UNKNOWN) {
			try_savetype = nds_get_save_info(cartridge).type;
		}

		load_or_create_savefile(pathname, try_savetype);
	}

	/* TODO: revert state if exception occured? */

	auto ctx = create_nds_ctx(arm7_bios.data(), arm9_bios.data(),
			firmware.data(), cartridge.data(), cartridge.size(),
			savefile.data(), savefile.size(), savetype, &config);
	if (direct_boot) {
		nds_direct_boot(ctx.get());
	} else {
		nds_firmware_boot(ctx.get());
	}

	nds = std::move(ctx);
}

void
nds_machine::shutdown()
{
	nds.reset();
	sync_files();
}

void
nds_machine::reboot(bool direct_boot)
{
	shutdown();
	boot(direct_boot);
}

void
nds_machine::run_frame()
{
	if (!nds)
		return;

	nds_run_frame(nds.get());
	if (nds->shutdown) {
		shutdown();
	}
}

bool
nds_machine::is_shutdown()
{
	return !nds;
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
nds_machine::update_touchscreen_state(
		int x, int y, bool down, bool quicktap, bool moved)
{
	if (!nds)
		return;

	x = std::clamp(x, 0, 255);
	y = std::clamp(y, 0, 191);
	nds_set_touchscreen_state(nds.get(), x, y, down, quicktap, moved);
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
	config.use_16_bit_audio = use_16_bit_audio;
}

void
nds_machine::sync_files()
{
	savefile.sync();
}

void
nds_machine::dump_profiler_report()
{
	if (!nds)
		return;

	nds_dump_prof(nds.get());
}

} // namespace twice
