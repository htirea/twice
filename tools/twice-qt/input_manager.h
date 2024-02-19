#ifndef TWICE_QT_INPUT_MANAGER_H
#define TWICE_QT_INPUT_MANAGER_H

#include "libtwice/nds/machine.h"

#include <QObject>

class ConfigManager;

class InputManager : public QObject {
	Q_OBJECT

      public:
	InputManager(ConfigManager *cfg, QObject *parent);
	~InputManager();
	twice::nds_button get_nds_button(int key);

      private slots:
	void rebind_nds_key(int varkey, const QVariant& v);

      private:
	std::map<int, twice::nds_button> key_to_nds;
	std::map<twice::nds_button, int> nds_to_key;
	ConfigManager *cfg{};
};

#endif
