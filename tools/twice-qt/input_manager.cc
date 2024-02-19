#include "input_manager.h"

#include "config_manager.h"

#include <QVariant>

using namespace twice;

static const std::map<nds_button, int> default_nds_to_key = {
	{ nds_button::A, Qt::Key_X },
	{ nds_button::B, Qt::Key_Z },
	{ nds_button::SELECT, Qt::Key_2 },
	{ nds_button::START, Qt::Key_1 },
	{ nds_button::RIGHT, Qt::Key_Right },
	{ nds_button::LEFT, Qt::Key_Left },
	{ nds_button::UP, Qt::Key_Up },
	{ nds_button::DOWN, Qt::Key_Down },
	{ nds_button::R, Qt::Key_W },
	{ nds_button::L, Qt::Key_Q },
	{ nds_button::X, Qt::Key_S },
	{ nds_button::Y, Qt::Key_A },
};

InputManager::InputManager(ConfigManager *cfg, QObject *parent)
	: QObject(parent), cfg(cfg)
{
	for (const auto& [button, default_key] : default_nds_to_key) {
		int varkey = (int)button + ConfigVariable::KEY_A;
		auto v = cfg->get(varkey);
		int key = v.isValid() ? v.toInt() : default_key;

		nds_to_key[button] = key;
		key_to_nds[key] = button;
	}

	connect(cfg, &ConfigManager::nds_key_set, this,
			&InputManager::rebind_nds_key);
}

InputManager::~InputManager() {}

nds_button
InputManager::get_nds_button(int key)
{
	auto it = key_to_nds.find(key);
	if (it == key_to_nds.end()) {
		return nds_button::NONE;
	}

	return it->second;
}

void
InputManager::rebind_nds_key(int varkey, const QVariant& v)
{
	nds_button button = (nds_button)(varkey - ConfigVariable::KEY_A);

	auto it = nds_to_key.find(button);
	if (it != nds_to_key.end()) {
		key_to_nds.erase(it->second);
		nds_to_key.erase(it);
	}

	if (v.isValid()) {
		int key = v.toInt();
		nds_to_key[button] = key;
		key_to_nds[key] = button;
	}
}
