#include "libtwice/nds/machine.h"
#include "libtwice/exception.h"
#include "libtwice/file/file.h"

#include "nds/nds.h"

#include <filesystem>
#include <unordered_map>
#include <utility>

namespace twice {

using namespace fs;

struct nds_machine::impl {
	impl(const nds_config& config);

	struct instance {
		file arm9_bios;
		file arm7_bios;
		file firmware;
		file cart;
		file save;
		nds_savetype savetype{ SAVETYPE_UNKNOWN };
		u32 gamecode{};
		std::unique_ptr<nds_ctx> nds;
	} curr, last;

	nds_config cfg;
	std::optional<nds_rtc_state> rtc_state;

	int save_instance(instance& to, instance& from);
};

nds_machine::impl::impl(const nds_config& cfg) : cfg(cfg) {}

int
nds_machine::impl::save_instance(instance& to, instance& from)
{
	int status = 0;

	try {
		to.arm9_bios = from.arm9_bios.dup();
		to.arm7_bios = from.arm7_bios.dup();
		to.firmware = from.firmware.dup();
		to.cart = from.cart.dup();
		to.save = from.save.dup();
	} catch (const twice_exception& err) {
		to.arm9_bios = file();
		to.arm7_bios = file();
		to.firmware = file();
		to.cart = file();
		to.save = file();
		status = 1;
	}

	to.savetype = from.savetype;
	to.gamecode = from.gamecode;
	to.nds = std::move(from.nds);

	return status;
}

nds_machine::nds_machine(const nds_config& config)
	: m(std::make_unique<impl>(config))
{
	try {
		auto pathname = config.arm9_bios_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "bios9.bin";
		}
		load_system_file(pathname, nds_file::ARM9_BIOS);
	} catch (const file_error& e) {
	}

	try {
		auto pathname = config.arm7_bios_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "bios7.bin";
		}
		load_system_file(pathname, nds_file::ARM7_BIOS);
	} catch (const file_error& e) {
	}

	try {
		auto pathname = config.firmware_path;
		if (pathname.empty()) {
			pathname = config.data_dir / "firmware.bin";
		}
		load_system_file(pathname, nds_file::FIRMWARE);
	} catch (const file_error& e) {
	}
}

nds_machine::~nds_machine()
{
	sync_files();
}

void
nds_machine::load_file(const std::filesystem::path& pathname, nds_file type)
{
	switch (type) {
	case nds_file::CART_ROM:
		load_cartridge(pathname);
		break;
	case nds_file::SAVE:
		load_savefile(pathname);
		break;
	case nds_file::ARM9_BIOS:
	case nds_file::ARM7_BIOS:
	case nds_file::FIRMWARE:
		load_system_file(pathname, type);
		break;
	case nds_file::UNKNOWN:
		throw twice_error("The file type is unknown.");
	}
}

void
nds_machine::unload_file(nds_file type)
{
	if (m->curr.nds) {
		throw twice_error("The file cannot be unloaded while the "
				  "machine is running.");
	}

	switch (type) {
	case nds_file::CART_ROM:
		eject_cartridge();
		break;
	case nds_file::SAVE:
		m->curr.save = file();
		break;
	case nds_file::ARM9_BIOS:
		m->curr.arm9_bios = file();
		break;
	case nds_file::ARM7_BIOS:
		m->curr.arm7_bios = file();
		break;
	case nds_file::FIRMWARE:
		m->curr.firmware = file();
		break;
	case nds_file::UNKNOWN:;
	}
}

void
nds_machine::load_system_file(
		const std::filesystem::path& pathname, nds_file type)
{
	if (m->curr.nds) {
		/* TODO: handle loading files while running */
		throw twice_error("The system file cannot be loaded while the "
				  "machine is running.");
	}

	if (type == nds_file::UNKNOWN) {
		throw twice_error("The system file type is unknown.");
	}

	if (type != nds_file::ARM9_BIOS && type != nds_file::ARM7_BIOS &&
			type != nds_file::FIRMWARE) {
		throw twice_error("The file type is not a system file.");
	}

	auto f = file(pathname, file::open_flags::READ);
	auto size = f.get_size();

	switch (type) {
	case nds_file::ARM9_BIOS:
		if (size != ARM9_BIOS_SIZE) {
			throw twice_error("Invalid ARM9 BIOS size.");
		}
		m->curr.arm9_bios = std::move(f);
		break;
	case nds_file::ARM7_BIOS:
		if (size != ARM7_BIOS_SIZE) {
			throw twice_error("Invalid ARM7 BIOS size.");
		}
		m->curr.arm7_bios = std::move(f);
		break;
	case nds_file::FIRMWARE:
		if (size != FIRMWARE_SIZE) {
			throw twice_error("Invalid NDS firmware size.");
		}
		m->curr.firmware = std::move(f);
		break;
	default:;
	}
}

void
nds_machine::load_cartridge(const std::filesystem::path& pathname)
{
	if (m->curr.nds) {
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
	if (f.read_exact_offset(0xC, buf, 4) < 0) {
		throw twice_error("Could not read cart gamecode.");
	}

	if (m->curr.cart) {
		eject_cartridge();
	}

	m->curr.cart = std::move(f);
	m->curr.gamecode = (u32)buf[3] << 24 | (u32)buf[2] << 16 |
	                   (u32)buf[1] << 8 | (u32)buf[0];
	LOG("loaded cartridge: %s, (0x%08X %c%c%c%c)\n", pathname.c_str(),
			m->curr.gamecode, buf[0], buf[1], buf[2], buf[3]);
}

void
nds_machine::eject_cartridge()
{
	if (m->curr.nds) {
		/* TODO: handle ejecting cartridge while running */
		throw twice_error("The cartridge cannot be ejected while the "
				  "machine is running.");
	}

	m->curr.cart = file();
	m->curr.save = file();
	m->curr.savetype = SAVETYPE_UNKNOWN;
}

void
nds_machine::load_savefile(const std::filesystem::path& pathname)
{
	if (m->curr.nds) {
		throw twice_error("The save file cannot be loaded while the "
				  "machine is running.");
	}

	m->curr.save = file(pathname, file::open_flags::READ_WRITE);
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

	m->curr.save = std::move(f);
	m->curr.savetype = savetype;

	LOG("created save file: %s\n", pathname.c_str());
}

void
nds_machine::autoload_savefile()
{
	if (!m->curr.cart)
		return;

	auto pathname = m->curr.cart.get_pathname().replace_extension(".sav");
	load_savefile(pathname);
}

void
nds_machine::autocreate_savefile()
{
	if (!m->curr.cart)
		return;

	auto pathname = m->curr.cart.get_pathname().replace_extension(".sav");
	auto from_db = nds_get_save_info(m->curr.gamecode).type;
	auto from_user = m->curr.savetype;
	auto type_to_use = from_user != SAVETYPE_UNKNOWN ? from_user : from_db;

	if (type_to_use == SAVETYPE_UNKNOWN) {
		throw twice_error(
				"Cannot create save file: unknown save type.");
	}

	if (type_to_use != SAVETYPE_NONE) {
		create_savefile(pathname, type_to_use);
	} else {
		m->curr.save = file();
		m->curr.savetype = SAVETYPE_NONE;
	}

	int from = 0;
	if (type_to_use == from_db) {
		from |= 1;
	}
	if (type_to_use == from_user) {
		from |= 2;
	}

	LOG("using save type: %s", nds_savetype_to_str(type_to_use));
	if (from & 1) {
		LOG(" (from db)");
	}
	if (from & 2) {
		LOG(" (explicitly set)");
	}
	LOG("\n");
}

bool
nds_machine::file_loaded(nds_file type)
{
	switch (type) {
	case nds_file::CART_ROM:
		return (bool)m->curr.cart;
	case nds_file::SAVE:
		return (bool)m->curr.save;
	case nds_file::ARM9_BIOS:
		return (bool)m->curr.arm9_bios;
	case nds_file::ARM7_BIOS:
		return (bool)m->curr.arm7_bios;
	case nds_file::FIRMWARE:
		return (bool)m->curr.firmware;
	default:
		return false;
	}
}

int
nds_machine::get_loaded_files()
{
	int r = 0;
	if ((bool)m->curr.cart) {
		r |= (int)nds_file::CART_ROM;
	}
	if ((bool)m->curr.save) {
		r |= (int)nds_file::SAVE;
	}
	if ((bool)m->curr.arm9_bios) {
		r |= (int)nds_file::ARM9_BIOS;
	}
	if ((bool)m->curr.arm7_bios) {
		r |= (int)nds_file::ARM7_BIOS;
	}
	if ((bool)m->curr.firmware) {
		r |= (int)nds_file::FIRMWARE;
	}

	return r;
}

int
nds_machine::sync_files() noexcept
{
	int status = 0;

	try {
		if (m->curr.nds) {
			nds_sync_files(m->curr.nds.get(), true);
		}
	} catch (const twice_exception& err) {
		status = 1;
	}

	return status;
}

nds_savetype
nds_machine::get_savetype()
{
	return m->curr.savetype;
}

void
nds_machine::set_savetype(nds_savetype savetype)
{
	if (m->curr.nds) {
		/* TODO: handle changing save type while running */
		throw twice_error("The save type cannot be changed while the "
				  "machine is running.");
	}

	m->curr.savetype = savetype;
}

void
nds_machine::check_savefile()
{
	if (!m->curr.cart || !m->curr.save)
		return;

	int from = 0;
	auto from_db = nds_get_save_info(m->curr.gamecode).type;
	auto from_user = m->curr.savetype;
	auto from_save = nds_savetype_from_size(m->curr.save.get_size());
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

	m->curr.savetype = type_to_use;

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
	if (!m->curr.cart)
		return;

	if (!m->curr.save) {
		try {
			autoload_savefile();
		} catch (const file_error& e) {
		}
	}

	if (!m->curr.save) {
		autocreate_savefile();
	} else {
		check_savefile();
	}
}

void
nds_machine::boot(bool direct_boot)
{
	if (m->curr.nds)
		return;

	if (!m->curr.arm9_bios || !m->curr.arm7_bios || !m->curr.firmware) {
		throw twice_error("The NDS system files must be loaded.");
	}

	if (direct_boot && !m->curr.cart) {
		throw twice_error("Cartridge must be loaded for direct "
				  "boot.");
	}

	prepare_savefile_for_execution();

	/* TODO: revert state if exception occured? */

	if (m->curr.cart && m->curr.savetype == SAVETYPE_UNKNOWN) {
		throw twice_error("Cannot boot machine: unknown save type.");
	}

	auto ctx = create_nds_ctx(m->curr.arm9_bios.pmap(),
			m->curr.arm7_bios.pmap(), m->curr.firmware.pmap(),
			m->curr.cart.pmap(),
			m->curr.savetype != SAVETYPE_NONE ? m->curr.save.dup()
							  : file(),
			m->curr.savetype, &m->cfg);
	if (direct_boot) {
		nds_direct_boot(ctx.get());
	} else {
		nds_firmware_boot(ctx.get());
	}

	if (!m->rtc_state) {
		set_rtc_state();
	}
	nds_set_rtc_state(ctx.get(), *m->rtc_state);
	unset_rtc_state();

	m->curr.nds = std::move(ctx);
}

int
nds_machine::shutdown() noexcept
{
	int status = sync_files();

	if (m->curr.nds) {
		if (m->save_instance(m->last, m->curr)) {
			status = 2;
		}
	}

	return status;
}

void
nds_machine::reboot(bool direct_boot)
{
	shutdown();
	boot(direct_boot);
}

void
nds_machine::restore_last_instance(bool save_current)
{
	std::swap(m->curr, m->last);
	if (!save_current) {
		m->last = {};
	}

	sync_files();
}

void
nds_machine::run_until_vblank(const nds_exec *in, nds_exec *out)
{
	if (!m->curr.nds) {
		throw twice_error("The machine is not running.");
	}

	nds_run(m->curr.nds.get(), run_mode::RUN_UNTIL_VBLANK, in, out);

	if (m->curr.nds->shutdown) {
		shutdown();
	}
}

bool
nds_machine::is_shutdown()
{
	return !m->curr.nds || m->curr.nds->shutdown;
}

bool
nds_machine::is_last_instance_shutdown()
{
	return !m->last.nds || m->last.nds->shutdown;
}

void
nds_machine::update_button_state(nds_button button, bool down)
{
	if (!m->curr.nds)
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
			m->curr.nds->keyinput &= ~BIT((int)button);
		} else {
			m->curr.nds->keyinput |= BIT((int)button);
		}
		break;
	case X:
		if (down) {
			m->curr.nds->extkeyin &= ~BIT(0);
		} else {
			m->curr.nds->extkeyin |= BIT(0);
		}
		break;
	case Y:
		if (down) {
			m->curr.nds->extkeyin &= ~BIT(1);
		} else {
			m->curr.nds->extkeyin |= BIT(1);
		}
		break;
	default:;
	}
}

void
nds_machine::update_touchscreen_state(
		int x, int y, bool down, bool quicktap, bool moved)
{
	if (!m->curr.nds)
		return;

	x = std::clamp(x, 0, 255);
	y = std::clamp(y, 0, 191);
	nds_set_touchscreen_state(
			m->curr.nds.get(), x, y, down, quicktap, moved);
}

void
nds_machine::set_rtc_state(const nds_rtc_state& s)
{
	if (m->curr.nds) {
		nds_set_rtc_state(m->curr.nds.get(), s);
	} else {
		m->rtc_state = s;
	}
}

void
nds_machine::set_rtc_state()
{
	using namespace std::chrono;

	auto tp = zoned_time{ current_zone(), system_clock::now() }
	                          .get_local_time();
	auto dp = floor<days>(tp);
	year_month_day date{ dp };
	hh_mm_ss time{ tp - dp };
	weekday wday{ dp };

	nds_rtc_state s{
		(int)date.year() % 100,
		(int)(unsigned)date.month(),
		(int)(unsigned)date.day(),
		(int)wday.iso_encoding() - 1,
		(int)time.hours().count(),
		(int)time.minutes().count(),
		(int)time.seconds().count(),
		true,
	};

	set_rtc_state(s);
}

void
nds_machine::unset_rtc_state()
{
	m->rtc_state.reset();
}

void
nds_machine::set_use_16_bit_audio(bool use_16_bit_audio)
{
	m->cfg.use_16_bit_audio = use_16_bit_audio;
}

void
nds_machine::set_interpolate_audio(bool interpolate_audio)
{
	m->cfg.interpolate_audio = interpolate_audio;
}

void
nds_machine::dump_profiler_report()
{
	if (!m->curr.nds)
		return;

	nds_dump_prof(m->curr.nds.get());
}

} // namespace twice
