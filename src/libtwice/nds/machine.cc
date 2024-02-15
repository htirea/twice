#include "libtwice/nds/machine.h"
#include "libtwice/exception.h"
#include "libtwice/file/file.h"

#include "common/date.h"
#include "nds/nds.h"

#include <filesystem>
#include <unordered_map>
#include <utility>

namespace twice {

using namespace fs;

struct nds_machine::impl {
	impl(const nds_config& config);

	nds_config cfg;
	file arm9_bios;
	file arm7_bios;
	file firmware;
	file cart;
	file save;
	nds_savetype savetype{ SAVETYPE_UNKNOWN };
	u32 gamecode{};
	std::unique_ptr<nds_ctx> nds;
};

nds_machine::impl::impl(const nds_config& cfg) : cfg(cfg) {}

nds_machine::nds_machine(const nds_config& config)
	: m(std::make_unique<impl>(config))
{
	try {
		auto pathname = config.arm9_bios_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "bios9.bin";
		}
		load_system_file(pathname, nds_system_file::ARM9_BIOS);
	} catch (const file_error& e) {
	}

	try {
		auto pathname = config.arm7_bios_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "bios7.bin";
		}
		load_system_file(pathname, nds_system_file::ARM7_BIOS);
	} catch (const file_error& e) {
	}

	try {
		auto pathname = config.firmware_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "firmware.bin";
		}
		load_system_file(pathname, nds_system_file::FIRMWARE);
	} catch (const file_error& e) {
	}
}

nds_machine::~nds_machine() = default;

void
nds_machine::load_system_file(
		const std::filesystem::path& pathname, nds_system_file type)
{
	if (m->nds) {
		/* TODO: handle loading files while running */
		throw twice_error("The system file cannot be loaded while the "
				  "machine is running.");
	}

	if (type == nds_system_file::UNKNOWN) {
		throw twice_error("The system file type is unknown.");
	}

	auto f = file(pathname, file::open_flags::READ);
	auto size = f.get_size();

	switch (type) {
	case nds_system_file::ARM9_BIOS:
		if (size != ARM9_BIOS_SIZE) {
			throw twice_error("Invalid ARM9 BIOS size.");
		}
		m->arm9_bios = std::move(f);
		break;
	case nds_system_file::ARM7_BIOS:
		if (size != ARM7_BIOS_SIZE) {
			throw twice_error("Invalid ARM7 BIOS size.");
		}
		m->arm7_bios = std::move(f);
		break;
	case nds_system_file::FIRMWARE:
		if (size != FIRMWARE_SIZE) {
			throw twice_error("Invalid NDS firmware size.");
		}
		m->firmware = std::move(f);
		break;
	default:;
	}
}

void
nds_machine::load_cartridge(const std::filesystem::path& pathname)
{
	if (m->nds) {
		/* TODO: handle loading cartridge while running */
		throw twice_error("The cartridge cannot be loaded while the "
				  "machine is running.");
	}

	auto f = file(pathname, file::open_flags::READ);
	auto size = f.get_size();

	if (size > MAX_CART_SIZE || size < MIN_CART_SIZE) {
		throw twice_error("Invalid cartridge size.");
	}

	u8 buf[4]{};
	auto count = f.read_from_offset(0xC, buf, 4);
	if (count != 4) {
		throw twice_error("Could not read cart gamecode.");
	}

	if (m->cart) {
		eject_cartridge();
	}

	m->cart = std::move(f);
	m->gamecode = (u32)buf[3] << 24 | (u32)buf[2] << 16 |
	              (u32)buf[1] << 8 | (u32)buf[0];
	LOG("loaded cartridge: %s, (0x%08X %c%c%c%c)\n", pathname.c_str(),
			m->gamecode, buf[0], buf[1], buf[2], buf[3]);
}

void
nds_machine::eject_cartridge()
{
	if (m->nds) {
		/* TODO: handle ejecting cartridge while running */
		throw twice_error("The cartridge cannot be ejected while the "
				  "machine is running.");
	}

	m->cart = file();
	m->save = file();
	m->savetype = SAVETYPE_UNKNOWN;
}

void
nds_machine::set_savetype(nds_savetype savetype)
{
	if (m->nds) {
		/* TODO: handle changing save type while running */
		throw twice_error("The save type cannot be changed while the "
				  "machine is running.");
	}

	m->savetype = savetype;
}

void
nds_machine::load_savefile(const std::filesystem::path& pathname)
{
	m->save = file(pathname, file::open_flags::READ_WRITE);
}

void
nds_machine::create_savefile(
		const std::filesystem::path& pathname, nds_savetype savetype)
{
	int flags = file::open_flags::READ_WRITE | file::open_flags::CREATE |
	            file::open_flags::NOREPLACE;
	auto f = file(pathname, flags);

	int err = f.truncate(nds_savetype_to_size(savetype));
	if (err) {
		throw twice_error("Could not truncate file.");
	}

	m->save = std::move(f);
	m->savetype = savetype;
}

void
nds_machine::autoload_savefile()
{
	if (!m->cart)
		return;

	auto pathname = m->cart.get_pathname().replace_extension(".sav");
	load_savefile(pathname);
}

void
nds_machine::autocreate_savefile()
{
	if (!m->cart)
		return;

	auto pathname = m->cart.get_pathname().replace_extension(".sav");
	auto from_db = nds_get_save_info(m->gamecode).type;
	auto from_user = m->savetype;
	auto type_to_use = from_user != SAVETYPE_UNKNOWN ? from_user : from_db;

	if (type_to_use == SAVETYPE_UNKNOWN) {
		throw twice_error(
				"Cannot create save file: unknown save type.");
	}

	if (type_to_use != SAVETYPE_NONE) {
		create_savefile(pathname, type_to_use);
	} else {
		m->save = file();
		m->savetype = SAVETYPE_NONE;
	}

	int from = 0;
	if (type_to_use == from_db) {
		from |= 1;
	}
	if (type_to_use == from_user) {
		from |= 2;
	}

	LOG("Created save file: using save type: %s",
			nds_savetype_to_str(type_to_use));
	if (from & 1) {
		LOG(" (from db)");
	}
	if (from & 2) {
		LOG(" (explicitly set)");
	}
	LOG("\n");
}

void
nds_machine::check_savefile()
{
	if (!m->cart || !m->save)
		return;

	int from = 0;
	auto from_db = nds_get_save_info(m->gamecode).type;
	auto from_user = m->savetype;
	auto from_save = nds_savetype_from_size(m->save.get_size());
	auto type_to_use = from_user != SAVETYPE_UNKNOWN ? from_user : from_db;

	if (type_to_use == from_db) {
		from |= 1;
	}
	if (type_to_use == from_user) {
		from |= 2;
	}

	if (type_to_use == SAVETYPE_UNKNOWN) {
		type_to_use = from_save;
	} else {
		if (from_save != type_to_use) {
			std::string msg = std::format(
					"Save file type: {}, does not match "
					"specified type: {}",
					nds_savetype_to_str(from_save),
					nds_savetype_to_str(type_to_use));
			if (from & 1) {
				msg += " (from db)";
			}
			if (from & 2) {
				msg += " (explicitly set)";
			}
			throw twice_error(msg);
		}
	}

	if (type_to_use == SAVETYPE_UNKNOWN) {
		throw twice_error("Invalid save file: unknown save type.");
	}

	m->savetype = type_to_use;

	from = 0;
	if (type_to_use == from_db) {
		from |= 1;
	}
	if (type_to_use == from_user) {
		from |= 2;
	}
	if (type_to_use == from_save) {
		from |= 4;
	}

	LOG("using save type: %s", nds_savetype_to_str(type_to_use));
	if (from & 1) {
		LOG(" (from db)");
	}
	if (from & 2) {
		LOG(" (explicitly set)");
	}
	if (from & 4) {
		LOG(" (from save file)");
	}
	LOG("\n");
}

void
nds_machine::prepare_savefile_for_execution()
{
	if (!m->cart)
		return;

	if (!m->save) {
		try {
			autoload_savefile();
		} catch (const file_error& e) {
		}
	}

	if (!m->save) {
		autocreate_savefile();
	} else {
		check_savefile();
	}
}

void
nds_machine::boot(bool direct_boot)
{
	if (m->nds)
		return;

	if (!m->arm9_bios || !m->arm7_bios || !m->firmware) {
		throw twice_error("The NDS system files must be loaded.");
	}

	if (direct_boot && !m->cart) {
		throw twice_error("Cartridge must be loaded for direct "
				  "boot.");
	}

	prepare_savefile_for_execution();

	/* TODO: revert state if exception occured? */

	if (m->savetype == SAVETYPE_UNKNOWN) {
		throw twice_error("Cannot boot machine: unknown save type.");
	}

	auto ctx = create_nds_ctx(m->arm9_bios.pmap(), m->arm7_bios.pmap(),
			m->firmware.pmap(), m->cart.pmap(), m->save.smap(),
			m->savetype, &m->cfg);
	if (direct_boot) {
		nds_direct_boot(ctx.get());
	} else {
		nds_firmware_boot(ctx.get());
	}

	m->nds = std::move(ctx);
}

void
nds_machine::shutdown()
{
	m->nds.reset();
	sync_files();
}

void
nds_machine::reboot(bool direct_boot)
{
	shutdown();
	boot(direct_boot);
}

void
nds_machine::run_until_vblank(const nds_exec *in, nds_exec *out)
{
	if (!m->nds) {
		throw twice_error("The machine is not running.");
	}

	nds_run(m->nds.get(), run_mode::RUN_UNTIL_VBLANK, in, out);

	if (m->nds->shutdown) {
		shutdown();
	}
}

void
nds_machine::run_frame()
{
	if (!m->nds) {
		throw twice_error("The machine is not running.");
	}

	nds_run(m->nds.get(), run_mode::RUN_UNTIL_VBLANK, nullptr, nullptr);

	if (m->nds->shutdown) {
		shutdown();
	}
}

bool
nds_machine::is_shutdown()
{
	return !m->nds;
}

u32 *
nds_machine::get_framebuffer()
{
	if (!m->nds)
		return nullptr;

	return m->nds->fb;
}

s16 *
nds_machine::get_audio_buffer()
{
	if (!m->nds)
		return nullptr;

	return m->nds->audio_buf.data();
}

u32
nds_machine::get_audio_buffer_size()
{
	if (!m->nds)
		return 0;

	return m->nds->last_audio_buf_size;
}

void
nds_machine::button_event(nds_button button, bool down)
{
	if (!m->nds)
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
			m->nds->keyinput &= ~BIT((int)button);
		} else {
			m->nds->keyinput |= BIT((int)button);
		}
		break;
	case X:
		if (down) {
			m->nds->extkeyin &= ~BIT(0);
		} else {
			m->nds->extkeyin |= BIT(0);
		}
		break;
	case Y:
		if (down) {
			m->nds->extkeyin &= ~BIT(1);
		} else {
			m->nds->extkeyin |= BIT(1);
		}
		break;
	default:;
	}
}

void
nds_machine::update_touchscreen_state(
		int x, int y, bool down, bool quicktap, bool moved)
{
	if (!m->nds)
		return;

	x = std::clamp(x, 0, 255);
	y = std::clamp(y, 0, 191);
	nds_set_touchscreen_state(m->nds.get(), x, y, down, quicktap, moved);
}

void
nds_machine::update_real_time_clock(int year, int month, int day, int weekday,
		int hour, int minute, int second)
{
	if (!m->nds)
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

	nds_set_rtc_time(m->nds.get(), year, month, day, weekday, hour, minute,
			second);
}

std::pair<double, double>
nds_machine::get_cpu_usage()
{
	if (!m->nds)
		return { 0, 0 };

	return { m->nds->arm9_usage, m->nds->arm7_usage };
}

void
nds_machine::set_use_16_bit_audio(bool use_16_bit_audio)
{
	m->cfg.use_16_bit_audio = use_16_bit_audio;
}

void
nds_machine::sync_files()
{
	if (m->nds) {
		nds_sync_files(m->nds.get());
	}

	/* TODO: sync files */
}

void
nds_machine::dump_profiler_report()
{
	if (!m->nds)
		return;

	nds_dump_prof(m->nds.get());
}

bool
nds_machine::cart_loaded()
{
	return (bool)m->cart;
}

} // namespace twice
