#ifndef TWICE_QT_ACTIONS_H
#define TWICE_QT_ACTIONS_H

#include <QAction>
#include <QActionGroup>

enum {
	ACTION_OPEN_ROM,
	ACTION_LOAD_SYSTEM_FILES,
	ACTION_INSERT_CART,
	ACTION_EJECT_CART,
	ACTION_LOAD_SAVE_FILE,
	ACTION_UNLOAD_SAVE_FILE,
	ACTION_SAVETYPE_AUTO,
	ACTION_SAVETYPE_NONE,
	ACTION_SAVETYPE_EEPROM_512B,
	ACTION_SAVETYPE_EEPROM_8K,
	ACTION_SAVETYPE_EEPROM_64K,
	ACTION_SAVETYPE_EEPROM_128K,
	ACTION_SAVETYPE_FLASH_256K,
	ACTION_SAVETYPE_FLASH_512K,
	ACTION_SAVETYPE_FLASH_1M,
	ACTION_SAVETYPE_FLASH_8M,
	ACTION_SAVETYPE_NAND,
	ACTION_RESET_TO_ROM,
	ACTION_RESET_TO_FIRMWARE,
	ACTION_SHUTDOWN,
	ACTION_TOGGLE_PAUSE,
	ACTION_TOGGLE_FASTFORWARD,
	ACTION_LINEAR_FILTERING,
	ACTION_LOCK_ASPECT_RATIO,
	ACTION_ORIENTATION_0,
	ACTION_ORIENTATION_1,
	ACTION_ORIENTATION_2,
	ACTION_ORIENTATION_3,
	ACTION_SCALE_1X,
	ACTION_SCALE_2X,
	ACTION_SCALE_3X,
	ACTION_SCALE_4X,
	ACTION_AUTO_RESIZE,
	ACTION_OPEN_SETTINGS,
	ACTION_TOTAL,
};

enum {
	ACTION_GROUP_ORIENTATION,
	ACTION_GROUP_SAVETYPE,
	ACTION_GROUP_TOTAL,
};

QAction *get_action(int id);
QAction *create_action(int id, QObject *parent);

template <typename F>
QAction *
create_action(int id, QObject *parent, F&& slot)
{
	auto action = create_action(id, parent);
	QObject::connect(action, &QAction::triggered, parent, slot);
	return action;
}

template <typename F>
QAction *
create_action(int id, QObject *parent, QObject *target, F&& slot)
{
	auto action = create_action(id, parent);
	QObject::connect(action, &QAction::triggered, target, slot);
	return action;
}

QActionGroup *get_action_group(int id);
QActionGroup *create_action_group(int id, QObject *parent);

int get_orientation();
bool linear_filtering_enabled();
bool lock_aspect_ratio();

#endif
