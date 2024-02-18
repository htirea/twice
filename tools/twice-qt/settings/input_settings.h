#ifndef TWICE_QT_INPUT_SETTINGS_H
#define TWICE_QT_INPUT_SETTINGS_H

#include <QWidget>

class InputControl;

class InputSettings : public QWidget {
	Q_OBJECT

      public:
	InputSettings(InputControl *input_ctrl, QWidget *parent);
	~InputSettings();

      private:
	InputControl *input_ctrl{};
};

#endif
