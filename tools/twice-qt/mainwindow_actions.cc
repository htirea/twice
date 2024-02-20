#include "mainwindow_actions.h"
#include "mainwindow.h"

#include "libtwice/exception.h"
#include "libtwice/nds/game_db.h"

#include <QActionGroup>
#include <QMenuBar>

using namespace MainWindowAction;
using namespace MainWindowMenu;

struct Action {
	int id;
};

struct Menu {
	int id;
};

struct Separator {};

struct ActionInfo {
	int id;
	const char *text;
	const char *tip;
	bool checkable{};
	std::optional<int> val;
	std::optional<QKeySequence::StandardKey> std_key;
};

struct MenuInfo {
	int id;
	const char *title;
	std::vector<std::variant<Action, Menu, Separator>> items;
};

struct GroupInfo {
	int id;
	std::vector<int> actions;
};

/* clang-format off */

static const std::vector<ActionInfo> action_data = {
{
	.id = OPEN_ROM,
	.text = "Open ROM",
	.tip = "Open a ROM file and start / restart emulation",
	.std_key = QKeySequence::Open,
},
{
	.id = LOAD_SYSTEM_FILES,
	.text = "Load system files",
	.tip = "Load the system files",
},
{
	.id = INSERT_CART,
	.text = "Insert cartridge",
	.tip = "Insert a cartridge",
},
{
	.id = EJECT_CART,
	.text = "Eject cartridge",
	.tip = "Eject the currently loaded cartridge",
},
{
	.id = LOAD_SAVE_FILE,
	.text = "Load save file",
	.tip = "Load a save file",
},
{
	.id = UNLOAD_SAVE_FILE,
	.text = "Unload save file",
	.tip = "Unload the currently loaded save file",
},
{
	.id = SET_SAVETYPE_AUTO,
	.text = "Auto",
	.tip = "Try to automatically detect the save type",
	.checkable = true,
	.val = twice::SAVETYPE_UNKNOWN,
},
{
	.id = SET_SAVETYPE_NONE,
	.text = "None",
	.tip = "Set the save type to none",
	.checkable = true,
	.val = twice::SAVETYPE_NONE,
},
{
	.id = SET_SAVETYPE_EEPROM_512B,
	.text = "EEPROM 512B",
	.tip = "Set the save type to EEPROM 512B",
	.checkable = true,
	.val = twice::SAVETYPE_EEPROM_512B,
},
{
	.id = SET_SAVETYPE_EEPROM_8K,
	.text = "EEPROM 8K",
	.tip = "Set the save type to EEPROM 8K",
	.checkable = true,
	.val = twice::SAVETYPE_EEPROM_8K,
},
{
	.id = SET_SAVETYPE_EEPROM_64K,
	.text = "EEPROM 64K",
	.tip = "Set the save type to EEPROM 64K",
	.checkable = true,
	.val = twice::SAVETYPE_EEPROM_64K,
},
{
	.id = SET_SAVETYPE_EEPROM_128K,
	.text = "EEPROM 128K",
	.tip = "Set the save type to EEPROM 128K",
	.checkable = true,
	.val = twice::SAVETYPE_EEPROM_128K,
},
{
	.id = SET_SAVETYPE_FLASH_256K,
	.text = "Flash 256K",
	.tip = "Set the save type to Flash 256K",
	.checkable = true,
	.val = twice::SAVETYPE_FLASH_256K,
},
{
	.id = SET_SAVETYPE_FLASH_512K,
	.text = "Flash 512K",
	.tip = "Set the save type to Flash 512K",
	.checkable = true,
	.val = twice::SAVETYPE_FLASH_512K,
},
{
	.id = SET_SAVETYPE_FLASH_1M,
	.text = "Flash 1M",
	.tip = "Set the save type to Flash 1M",
	.checkable = true,
	.val = twice::SAVETYPE_FLASH_1M,
},
{
	.id = SET_SAVETYPE_FLASH_8M,
	.text = "Flash 8M",
	.tip = "Set the save type to Flash 8M",
	.checkable = true,
	.val = twice::SAVETYPE_FLASH_8M,
},
{
	.id = SET_SAVETYPE_NAND,
	.text = "NAND",
	.tip = "Not implemented",
	.checkable = true,
	.val = twice::SAVETYPE_NAND,
},
{
	.id = RESET_TO_ROM,
	.text = "Reset to ROM",
	.tip = "Reset emulation to the ROM (direct boot)",
},
{
	.id = RESET_TO_FIRMWARE,
	.text = "Reset to firmware",
	.tip = "Reset emulation to firmware (firmware boot)",
},
{
	.id = SHUTDOWN,
	.text = "Shutdown",
	.tip = "Shutdown the virtual machine",
},
{
	.id = TOGGLE_PAUSE,
	.text = "Pause",
	.tip = "Pause the emulation",
	.checkable = true,
},
{
	.id = TOGGLE_FASTFORWARD,
	.text = "Fast forward",
	.tip = "Fast forward the emulation",
	.checkable = true,
},
{
	.id = LINEAR_FILTERING,
	.text = "Linear filtering",
	.tip = "Enable linear filtering",
	.checkable = true,
},
{
	.id = LOCK_ASPECT_RATIO,
	.text = "Lock aspect ratio",
	.tip = "Lock the aspect ratio",
	.checkable = true,
},
{
	.id = ROTATE_CLOCKWISE,
	.text = "90° clockwise",
	.tip = "Rotate the screen by 90° clockwise",
	.val = -3,
},
{
	.id = ROTATE_HALFCIRCLE,
	.text = "180°",
	.tip = "Rotate the screen by 180°",
	.val = -2,
},
{
	.id = ROTATE_ANTICLOCKWISE,
	.text = "90° anticlockwise",
	.tip = "Rotate the screen by 90° anticlockwise",
	.val = -1,
},
{
	.id = ORIENTATION_0,
	.text = "0°",
	.tip = "Set the orientation to 0°",
	.checkable = true,
	.val = 0,
},
{
	.id = ORIENTATION_1,
	.text = "90°",
	.tip = "Set the orientation to 90°",
	.checkable = true,
	.val = 1,
},
{
	.id = ORIENTATION_2,
	.text = "180°",
	.tip = "Set the orientation to 180°",
	.checkable = true,
	.val = 2,
},
{
	.id = ORIENTATION_3,
	.text = "270°",
	.tip = "Set the orientation to 270°",
	.checkable = true,
	.val = 3,
},
{
	.id = SCALE_1X,
	.text = "1x",
	.tip = "Scale emulated screen by 1x",
	.val = 1,
},
{
	.id = SCALE_2X,
	.text = "2x",
	.tip = "Scale emulated screen by 2x",
	.val = 2,
},
{
	.id = SCALE_3X,
	.text = "3x",
	.tip = "Scale emulated screen by 3x",
	.val = 3,
},
{
	.id = SCALE_4X,
	.text = "4x",
	.tip = "Scale emulated screen by 4x",
	.val = 4,
},
{
	.id = AUTO_RESIZE,
	.text = "Auto resize",
	.tip = "Resize the window to match the emulated aspect ratio",
},
{
	.id = OPEN_SETTINGS,
	.text = "Settings",
	.tip = "Open the settings window",
},
};

static const std::vector<int> menubar_menus = {
	FILE_MENU,
	EMULATION_MENU,
	VIDEO_MENU,
	TOOLS_MENU,
};

static const std::vector<MenuInfo> menu_data = {
{
	.id = FILE_MENU,
	.title = "File",
	.items = {
		Action{OPEN_ROM},
		Action{LOAD_SYSTEM_FILES},
		Separator{},
		Action{INSERT_CART},
		Action{EJECT_CART},
		Separator{},
		Action{LOAD_SAVE_FILE},
		Action{UNLOAD_SAVE_FILE},
		Menu{SAVETYPE_MENU}
	},
},
{
	.id = SAVETYPE_MENU,
	.title = "Save type",
	.items = {
		Action{SET_SAVETYPE_AUTO},
		Action{SET_SAVETYPE_NONE},
		Action{SET_SAVETYPE_EEPROM_512B},
		Action{SET_SAVETYPE_EEPROM_8K},
		Action{SET_SAVETYPE_EEPROM_64K},
		Action{SET_SAVETYPE_EEPROM_128K},
		Action{SET_SAVETYPE_FLASH_256K},
		Action{SET_SAVETYPE_FLASH_512K},
		Action{SET_SAVETYPE_FLASH_1M},
		Action{SET_SAVETYPE_FLASH_8M},
		Action{SET_SAVETYPE_NAND},
	},
},
{
	.id = EMULATION_MENU,
	.title = "Emulation",
	.items = {
		Action{RESET_TO_ROM},
		Action{RESET_TO_FIRMWARE},
		Action{SHUTDOWN},
		Separator{},
		Action{TOGGLE_PAUSE}, 
		Action{TOGGLE_FASTFORWARD},
	},
},
{
	.id = VIDEO_MENU,
	.title = "Video",
	.items = {
		Action{AUTO_RESIZE},
		Menu{SCALE_MENU},
		Menu{ROTATION_MENU},
		Menu{ORIENTATION_MENU},
		Separator{},
		Action{LINEAR_FILTERING},
		Action{LOCK_ASPECT_RATIO},
	},
},
{
	.id = SCALE_MENU,
	.title = "Scale",
	.items = {
		Action{SCALE_1X},
		Action{SCALE_2X},
		Action{SCALE_3X},
		Action{SCALE_4X}
	},
},
{
	.id = ROTATION_MENU,
	.title = "Rotate",
	.items = {
		Action{ROTATE_CLOCKWISE},
		Action{ROTATE_ANTICLOCKWISE},
		Action{ROTATE_HALFCIRCLE},
	},
},
{
	.id = ORIENTATION_MENU,
	.title = "Orientation",
	.items = {
		Action{ORIENTATION_0},
		Action{ORIENTATION_1},
		Action{ORIENTATION_2},
		Action{ORIENTATION_3},
	},
},
{
	.id = TOOLS_MENU,
	.title = "Tools",
	.items = {
		Action{OPEN_SETTINGS}
	},
},
};

static const std::vector<GroupInfo> group_data = {
{
	.id = SAVETYPE_GROUP,
	.actions = {
		SET_SAVETYPE_AUTO,
		SET_SAVETYPE_NONE,
		SET_SAVETYPE_EEPROM_512B,
		SET_SAVETYPE_EEPROM_8K,
		SET_SAVETYPE_EEPROM_64K,
		SET_SAVETYPE_EEPROM_128K,
		SET_SAVETYPE_FLASH_256K,
		SET_SAVETYPE_FLASH_512K,
		SET_SAVETYPE_FLASH_1M,
		SET_SAVETYPE_FLASH_8M,
		SET_SAVETYPE_NAND,
	},
},
{
	.id = SCALE_GROUP,
	.actions = {
		SCALE_1X,
		SCALE_2X,
		SCALE_3X,
		SCALE_4X,
	},
},
{
	.id = ROTATION_GROUP,
	.actions = {
		ROTATE_CLOCKWISE,
		ROTATE_HALFCIRCLE,
		ROTATE_ANTICLOCKWISE,
	},
},
{
	.id = ORIENTATION_GROUP,
	.actions = {
		ORIENTATION_0,
		ORIENTATION_1,
		ORIENTATION_2,
		ORIENTATION_3,
	},
},
};

/* clang-format on */

void
MainWindow::init_actions()
{
	for (int id = 0; id < NUM_ACTIONS; id++) {
		auto const& a = action_data.at(id);
		if (a.id != id) {
			throw twice::twice_error("Invalid action id.");
		}

		auto action = new QAction(this);
		action->setText(tr(a.text));
		action->setToolTip(tr(a.tip));
		action->setCheckable(a.checkable);

		if (a.val) {
			action->setData(*a.val);
		}

		if (a.std_key) {
			action->setShortcut(*a.std_key);
		}

		actions.push_back(action);
	}

	for (int id = 0; id < NUM_GROUPS; id++) {
		auto const& g = group_data.at(id);
		if (g.id != id) {
			throw twice::twice_error("Invalid group id.");
		}

		auto group = new QActionGroup(this);
		for (int action_id : g.actions) {
			auto action = actions.at(action_id);
			group->addAction(action);
		}

		action_groups.push_back(group);
	}

	actions.at(SET_SAVETYPE_NAND)->setEnabled(false);
}

void
MainWindow::init_menus()
{
	for (int id = 0; id < NUM_MENUS; id++) {
		auto const& m = menu_data.at(id);
		if (m.id != id) {
			throw twice::twice_error("Invalid menu id.");
		}

		auto menu = new QMenu(this);
		menu->setTitle(tr(m.title));
		menus.push_back(menu);
	}

	for (int id = 0; id < NUM_MENUS; id++) {
		auto const& m = menu_data.at(id);
		auto menu = menus.at(id);

		for (const auto& item : m.items) {
			if (auto it = std::get_if<Action>(&item)) {
				auto action = actions.at(it->id);
				menu->addAction(action);
			} else if (auto it = std::get_if<Menu>(&item)) {
				auto submenu = menus.at(it->id);
				menu->addMenu(submenu);
			} else {
				menu->addSeparator();
			}
		}
	}

	auto menubar = menuBar();
	for (int id : menubar_menus) {
		menubar->addMenu(menus.at(id));
	}

	typedef void (MainWindow::*F)();
	typedef void (MainWindow::*F1)(bool);
	typedef void (MainWindow::*F_int)(int);

	std::vector<std::pair<int, F>> funcs0 = {
		{ OPEN_ROM, &MainWindow::open_rom },
		{ LOAD_SYSTEM_FILES, &MainWindow::open_system_files },
		{ INSERT_CART, &MainWindow::insert_cart },
		{ EJECT_CART, &MainWindow::eject_cart },
		{ LOAD_SAVE_FILE, &MainWindow::load_save_file },
		{ UNLOAD_SAVE_FILE, &MainWindow::unload_save_file },
		{ RESET_TO_ROM, &MainWindow::reset_to_rom },
		{ RESET_TO_FIRMWARE, &MainWindow::reset_to_firmware },
		{ SHUTDOWN, &MainWindow::shutdown_emulation },
		{ AUTO_RESIZE, &MainWindow::auto_resize_display },
		{ OPEN_SETTINGS, &MainWindow::open_settings },
	};

	std::vector<std::pair<int, F1>> funcs1 = {
		{ TOGGLE_PAUSE, &MainWindow::toggle_pause },
		{ TOGGLE_FASTFORWARD, &MainWindow::toggle_fastforward },
		{ LINEAR_FILTERING, &MainWindow::toggle_linear_filtering },
		{ LOCK_ASPECT_RATIO, &MainWindow::toggle_lock_aspect_ratio },
	};

	std::vector<std::pair<int, F_int>> funcs_int = {
		{ SAVETYPE_GROUP, &MainWindow::set_savetype },
		{ SCALE_GROUP, &MainWindow::set_scale },
		{ ROTATION_GROUP, &MainWindow::set_orientation },
		{ ORIENTATION_GROUP, &MainWindow::set_orientation },
	};

	for (const auto& [id, f] : funcs0) {
		connect(actions.at(id), &QAction::triggered, this, f);
	}

	for (const auto& [id, f] : funcs1) {
		connect(actions.at(id), &QAction::triggered, this, f);
	}

	for (const auto& p : funcs_int) {
		auto id = p.first;
		auto f = p.second;
		connect(action_groups.at(id), &QActionGroup::triggered, this,
				[=, this](QAction *action) {
					(this->*f)(action->data().toInt());
				});
	}
}
