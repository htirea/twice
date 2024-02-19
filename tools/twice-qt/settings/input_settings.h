#ifndef TWICE_QT_INPUT_SETTINGS_H
#define TWICE_QT_INPUT_SETTINGS_H

#include <QWidget>

class ConfigManager;

class InputSettings : public QWidget {
	Q_OBJECT

      public:
	InputSettings(ConfigManager *cfg, QWidget *parent);
	~InputSettings();

      private:
	ConfigManager *cfg{};
};

#endif
