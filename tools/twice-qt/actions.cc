#include "actions.h"

#include "libtwice/exception.h"
#include "libtwice/nds/game_db.h"

using namespace twice;

struct ActionInfo {
	int id;
	const char *text;
	const char *tip;
	bool checkable{};
	int val{};
	QKeySequence::StandardKey std_key{ QKeySequence::UnknownKey };
};

/* clang-format off */
static const std::vector<ActionInfo> action_data = {
{
	.id = ACTION_OPEN_ROM,
	.text = "Open ROM",
	.tip = "Open a ROM file and start / restart emulation",
	.std_key = QKeySequence::Open,
},
{
	.id = ACTION_LOAD_SYSTEM_FILES,
	.text = "Load system files",
	.tip = "Load the system files",
},
{
	.id = ACTION_INSERT_CART,
	.text = "Insert cartridge",
	.tip = "Insert a cartridge",
},
{
	.id = ACTION_EJECT_CART,
	.text = "Eject cartridge",
	.tip = "Eject the currently loaded cartridge",
},
{
	.id = ACTION_LOAD_SAVE_FILE,
	.text = "Load save file",
	.tip = "Load a save file",
},
{
	.id = ACTION_UNLOAD_SAVE_FILE,
	.text = "Unload save file",
	.tip = "Unload the currently loaded save file",
},
{
	.id = ACTION_SAVETYPE_AUTO,
	.text = "Auto",
	.tip = "Try to automatically detect the save type",
	.checkable = true,
	.val = SAVETYPE_UNKNOWN,
},
{
	.id = ACTION_SAVETYPE_NONE,
	.text = "None",
	.tip = "Set the save type to none",
	.checkable = true,
	.val = SAVETYPE_NONE,
},
{
	.id = ACTION_SAVETYPE_EEPROM_512B,
	.text = "EEPROM 512B",
	.tip = "Set the save type to EEPROM 512B",
	.checkable = true,
	.val = SAVETYPE_EEPROM_512B,
},
{
	.id = ACTION_SAVETYPE_EEPROM_8K,
	.text = "EEPROM 8K",
	.tip = "Set the save type to EEPROM 8K",
	.checkable = true,
	.val = SAVETYPE_EEPROM_8K,
},
{
	.id = ACTION_SAVETYPE_EEPROM_64K,
	.text = "EEPROM 64K",
	.tip = "Set the save type to EEPROM 64K",
	.checkable = true,
	.val = SAVETYPE_EEPROM_64K,
},
{
	.id = ACTION_SAVETYPE_EEPROM_128K,
	.text = "EEPROM 128K",
	.tip = "Set the save type to EEPROM 128K",
	.checkable = true,
	.val = SAVETYPE_EEPROM_128K,
},
{
	.id = ACTION_SAVETYPE_FLASH_256K,
	.text = "Flash 256K",
	.tip = "Set the save type to Flash 256K",
	.checkable = true,
	.val = SAVETYPE_FLASH_256K,
},
{
	.id = ACTION_SAVETYPE_FLASH_512K,
	.text = "Flash 512K",
	.tip = "Set the save type to Flash 512K",
	.checkable = true,
	.val = SAVETYPE_FLASH_512K,
},
{
	.id = ACTION_SAVETYPE_FLASH_1M,
	.text = "Flash 1M",
	.tip = "Set the save type to Flash 1M",
	.checkable = true,
	.val = SAVETYPE_FLASH_1M,
},
{
	.id = ACTION_SAVETYPE_FLASH_8M,
	.text = "Flash 8M",
	.tip = "Set the save type to Flash 8M",
	.checkable = true,
	.val = SAVETYPE_FLASH_8M,
},
{
	.id = ACTION_SAVETYPE_NAND,
	.text = "NAND",
	.tip = "Not implemented",
	.checkable = true,
	.val = SAVETYPE_NAND,
},
{
	.id = ACTION_RESET_TO_ROM,
	.text = "Reset to ROM",
	.tip = "Reset emulation to the ROM (direct boot)",
},
{
	.id = ACTION_RESET_TO_FIRMWARE,
	.text = "Reset to firmware",
	.tip = "Reset emulation to firmware (firmware boot)",
},
{
	.id = ACTION_SHUTDOWN,
	.text = "Shutdown",
	.tip = "Shutdown the virtual machine",
},
{
	.id = ACTION_TOGGLE_PAUSE,
	.text = "Pause",
	.tip = "Pause the emulation",
	.checkable = true,
},
{
	.id = ACTION_TOGGLE_FASTFORWARD,
	.text = "Fast forward",
	.tip = "Fast forward the emulation",
	.checkable = true,
},
{
	.id = ACTION_LINEAR_FILTERING,
	.text = "Linear filtering",
	.tip = "Enable linear filtering",
	.checkable = true,
},
{
	.id = ACTION_LOCK_ASPECT_RATIO,
	.text = "Lock aspect ratio",
	.tip = "Lock the aspect ratio",
	.checkable = true,
},
{
	.id = ACTION_ORIENTATION_0,
	.text = "0°",
	.tip = "Rotate the screen 0 degrees",
	.checkable = true,
	.val = 0,
},
{
	.id = ACTION_ORIENTATION_1,
	.text = "90° clockwise",
	.tip = "Rotate the screen 90 degrees clockwise",
	.checkable = true,
	.val = 1,
},
{
	.id = ACTION_ORIENTATION_2,
	.text = "180°",
	.tip = "Rotate the screen 180 degrees",
	.checkable = true,
	.val = 2,
},
{
	.id = ACTION_ORIENTATION_3,
	.text = "90° anticlockwise",
	.tip = "Rotate the screen 90 degrees anticlockwise",
	.checkable = true,
	.val = 3,
},
{
	.id = ACTION_SCALE_1X,
	.text = "1x",
	.tip = "Scale emulated screen by 1x",
	.val = 1,
},
{
	.id = ACTION_SCALE_2X,
	.text = "2x",
	.tip = "Scale emulated screen by 2x",
	.val = 2,
},
{
	.id = ACTION_SCALE_3X,
	.text = "3x",
	.tip = "Scale emulated screen by 3x",
	.val = 3,
},
{
	.id = ACTION_SCALE_4X,
	.text = "4x",
	.tip = "Scale emulated screen by 4x",
	.val = 4,
},
{
	.id = ACTION_AUTO_RESIZE,
	.text = "Auto resize",
	.tip = "Resize the window to match the emulated aspect ratio",
},
{
	.id = ACTION_OPEN_SETTINGS,
	.text = "Settings",
	.tip = "Open the settings window",
},
};

static std::array<QAction *, ACTION_TOTAL> actions{};

static std::array<QActionGroup *, ACTION_GROUP_TOTAL> action_groups{};

/* clang-format on */

QAction *
get_action(int id)
{
	return actions[id];
}

QAction *
create_action(int id, QObject *parent)
{
	auto const& a = action_data[id];
	if (a.id != id) {
		throw twice_error("Invalid action id.");
	}

	QAction *action = new QAction(parent);
	action->setText(QObject::tr(a.text));
	action->setToolTip(QObject::tr(a.tip));
	action->setCheckable(a.checkable);
	action->setData(a.val);

	if (a.std_key != QKeySequence::UnknownKey) {
		action->setShortcut(a.std_key);
	}

	return actions[id] = action;
}

QActionGroup *
get_action_group(int id)
{
	return action_groups[id];
}

QActionGroup *
create_action_group(int id, QObject *parent)
{
	if (id < 0 || id >= ACTION_GROUP_TOTAL) {
		throw twice_error("Invalid action group id.");
	}

	return action_groups[id] = new QActionGroup(parent);
}

int
get_orientation()
{
	auto action = get_action_group(ACTION_GROUP_ORIENTATION)
	                              ->checkedAction();
	if (!action)
		return 0;

	return action->data().toInt();
}

bool
linear_filtering_enabled()
{
	return actions[ACTION_LINEAR_FILTERING]->isChecked();
}

bool
lock_aspect_ratio()
{
	return actions[ACTION_LOCK_ASPECT_RATIO]->isChecked();
}
