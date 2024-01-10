#ifndef TWICE_QT_EMULATOR_EVENTS_H
#define TWICE_QT_EMULATOR_EVENTS_H

#include <string>
#include <variant>

#include "libtwice/nds/machine.h"

namespace twice {

struct DummyEvent {};

struct QuitEvent {};

struct LoadFileEvent {
	enum {
		ARM9_BIOS,
		ARM7_BIOS,
		FIRMWARE,
	} type;

	std::string pathname;
};

struct LoadROMEvent {
	std::string pathname;
};

struct SetSavetypeEvent {
	nds_savetype type;
};

struct PauseEvent {};

struct ResumeEvent {};

struct StopEvent {};

struct SetFastForwardEvent {
	bool fast_forward;
};

struct ResetEvent {
	bool direct;
};

struct RotateEvent {
	int orientation;
};

struct ButtonEvent {
	nds_button which;
	bool down;
};

struct TouchEvent {
	int x;
	int y;
	bool down;
};

struct UpdateRTCEvent {
	int year;
	int month;
	int day;
	int weekday;
	int hour;
	int minute;
	int second;
};

using Event = std::variant<DummyEvent, QuitEvent, LoadFileEvent, LoadROMEvent,
		SetSavetypeEvent, PauseEvent, ResumeEvent, StopEvent,
		SetFastForwardEvent, ResetEvent, RotateEvent, ButtonEvent,
		TouchEvent, UpdateRTCEvent>;

} // namespace twice

#endif
