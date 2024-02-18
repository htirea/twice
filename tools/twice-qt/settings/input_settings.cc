#include "input_settings.h"

InputSettings::InputSettings(InputControl *input_ctrl, QWidget *parent)
	: QWidget(parent), input_ctrl(input_ctrl)
{
}

InputSettings::~InputSettings() {}
