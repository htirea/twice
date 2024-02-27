#ifndef TWICE_QT_INPUT_MANAGER_H
#define TWICE_QT_INPUT_MANAGER_H

#include "libtwice/nds/machine.h"

#include <QObject>

class ConfigManager;
class EmulatorThread;

class InputManager : public QObject {
	Q_OBJECT

      public:
	InputManager(ConfigManager *cfg, EmulatorThread *emu_thread,
			QObject *parent);
	~InputManager();
	void process_events();
	int get_nds_button(int code, int which);
	void set_nds_button(int button, bool down);

      private:
	void add_controller(int id);
	void remove_controller(int id);

      private slots:
	void rebind_nds_button(int key, const QVariant& v);

      private:
	struct impl;
	std::unique_ptr<impl> m;
	ConfigManager *cfg{};
	EmulatorThread *emu_thread{};
};

#endif
