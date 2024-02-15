#include "actions.h"

#include "libtwice/exception.h"

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
	.id = ACTION_LOAD_SAVE_FILE,
	.text = "Load save file",
	.tip = "Load a save file",
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
