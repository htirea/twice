#include "input_manager.h"

#include "config_manager.h"
#include "emulator_thread.h"
#include "libtwice/exception.h"

#include <QVariant>
#include <SDL.h>

#include <iostream>
#include <set>

using namespace twice;

/* clang-format off */
static const std::map<nds_button, int> default_nds_to_code[2] = {
{
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
},
{
	{ nds_button::A, SDL_CONTROLLER_BUTTON_B, },
	{ nds_button::B, SDL_CONTROLLER_BUTTON_A },
	{ nds_button::SELECT, SDL_CONTROLLER_BUTTON_BACK },
	{ nds_button::START, SDL_CONTROLLER_BUTTON_START },
	{ nds_button::RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
	{ nds_button::LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT },
	{ nds_button::UP, SDL_CONTROLLER_BUTTON_DPAD_UP },
	{ nds_button::DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN },
	{ nds_button::R, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
	{ nds_button::L, SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
	{ nds_button::X, SDL_CONTROLLER_BUTTON_Y },
	{ nds_button::Y, SDL_CONTROLLER_BUTTON_X },
}
};

/* clang-format on */

struct InputManager::impl {
	std::map<int, nds_button> code_to_nds_button[2];
	std::map<nds_button, int> nds_button_to_code[2];
	std::set<SDL_JoystickID> joysticks;
};

InputManager::InputManager(ConfigManager *cfg, EmulatorThread *emu_thread,
		QObject *parent)
	: QObject(parent),
	  m(std::make_unique<impl>()),
	  cfg(cfg),
	  emu_thread(emu_thread)
{
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)) {
		throw twice_error("SDL: failed to initalize.");
	}

	int num_joysticks = SDL_NumJoysticks();
	for (int i = 0; i < num_joysticks; i++) {
		if (SDL_IsGameController(i)) {
			add_controller(i);
		}
	}

	for (int which = 0; which < 2; which++) {
		int base_key = which == 0 ? ConfigVariable::KEY_A
		                          : ConfigVariable::GC_A;
		for (const auto& [button, default_code] :
				default_nds_to_code[which]) {
			int key = base_key + (int)button;
			auto v = cfg->get(key);
			int code = v.isValid() ? v.toInt() : default_code;
			m->nds_button_to_code[which][button] = code;
			m->code_to_nds_button[which][code] = button;
		}
	}

	connect(cfg, &ConfigManager::nds_bind_set, this,
			&InputManager::rebind_nds_button);
}

InputManager::~InputManager()
{
	while (!m->joysticks.empty()) {
		auto id = *m->joysticks.begin();
		remove_controller(id);
	}

	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void
InputManager::process_events()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_CONTROLLERDEVICEADDED:
			add_controller(ev.cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			remove_controller(ev.cdevice.which);
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		{
			auto button = get_nds_button(ev.cbutton.button, 1);
			if (button != nds_button::NONE) {
				emu_thread->push_event(
						Event::Button{ button, true });
			}
			break;
		}
		case SDL_CONTROLLERBUTTONUP:
		{
			auto button = get_nds_button(ev.cbutton.button, 1);
			if (button != nds_button::NONE) {
				emu_thread->push_event(Event::Button{
						button, false });
			}
			break;
		}
		}
	}
}

nds_button
InputManager::get_nds_button(int code, int which)
{
	auto it = m->code_to_nds_button[which].find(code);
	if (it == m->code_to_nds_button[which].end()) {
		return nds_button::NONE;
	}

	return it->second;
}

void
InputManager::add_controller(int id)
{
	SDL_GameController *gc = SDL_GameControllerOpen(id);
	if (gc) {
		SDL_Joystick *joy = SDL_GameControllerGetJoystick(gc);
		SDL_JoystickID joy_id = SDL_JoystickInstanceID(joy);
		m->joysticks.insert(joy_id);
	}
}

void
InputManager::remove_controller(int id)
{
	m->joysticks.erase(id);
	SDL_GameController *gc = SDL_GameControllerFromInstanceID(id);
	if (gc) {
		SDL_GameControllerClose(gc);
	}
}

void
InputManager::rebind_nds_button(int key, const QVariant& v)
{
	if (!v.isValid())
		return;

	auto button = (nds_button)((key - ConfigVariable::KEY_A) %
				   (int)nds_button::NONE);
	int which = (key - ConfigVariable::KEY_A) / (int)nds_button::NONE;
	if (which > 2)
		return;

	auto it = m->nds_button_to_code[which].find(button);
	if (it != m->nds_button_to_code[which].end()) {
		m->code_to_nds_button[which].erase(it->second);
		m->nds_button_to_code[which].erase(it);
	}

	int code = v.toInt();
	m->nds_button_to_code[which][button] = code;
	m->code_to_nds_button[which][code] = button;
}
