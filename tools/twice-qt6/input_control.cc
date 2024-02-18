#include "input_control.h"

#include <QSettings>
#include <utility>

using namespace twice;

static const std::map<nds_button, QString> button_to_str = {
	{ nds_button::A, "key_A" },
	{ nds_button::B, "key_B" },
	{ nds_button::SELECT, "key_SELECT" },
	{ nds_button::START, "key_START" },
	{ nds_button::RIGHT, "key_RIGHT" },
	{ nds_button::LEFT, "key_LEFT" },
	{ nds_button::UP, "key_UP" },
	{ nds_button::DOWN, "key_DOWN" },
	{ nds_button::R, "key_R" },
	{ nds_button::L, "key_L" },
	{ nds_button::X, "key_X" },
	{ nds_button::Y, "key_Y" },
	{ nds_button::NONE, "key_NONE" },
};

static const std::map<int, nds_button> default_key_to_nds = {
	{ Qt::Key_X, nds_button::A },
	{ Qt::Key_Z, nds_button::B },
	{ Qt::Key_2, nds_button::SELECT },
	{ Qt::Key_1, nds_button::START },
	{ Qt::Key_Right, nds_button::RIGHT },
	{ Qt::Key_Left, nds_button::LEFT },
	{ Qt::Key_Up, nds_button::UP },
	{ Qt::Key_Down, nds_button::DOWN },
	{ Qt::Key_W, nds_button::R },
	{ Qt::Key_Q, nds_button::L },
	{ Qt::Key_S, nds_button::X },
	{ Qt::Key_A, nds_button::Y },
};

InputControl::InputControl(QObject *parent) : QObject(parent)
{
	QSettings settings;

	for (const auto& [button, str] : button_to_str) {
		if (settings.contains(str)) {
			auto key = settings.value(str).toInt();
			set_nds_mapping(button, key);
		}
	}
}

InputControl::~InputControl()
{
	QSettings settings;

	for (const auto& [key, button] : key_to_nds) {
		auto str = button_to_str.at(button);
		settings.setValue(str, key);
	}
}

void
InputControl::set_nds_mapping(nds_button button, int key)
{
	key_to_nds[key] = button;
}

nds_button
InputControl::get_nds_mapping(int key)
{
	auto it = key_to_nds.find(key);
	if (it != key_to_nds.end()) {
		return it->second;
	}

	auto def_it = default_key_to_nds.find(key);
	if (def_it != default_key_to_nds.end()) {
		return def_it->second;
	}

	return nds_button::NONE;
}
