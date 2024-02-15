#ifndef TWICE_QT_EVENTS_H
#define TWICE_QT_EVENTS_H

#include "libtwice/nds/machine.h"

#include <QString>

#include <filesystem>

struct EmptyEvent {};

struct LoadROMEvent {
	QString pathname;
};

struct LoadSystemFileEvent {
	QString pathname;
	twice::nds_system_file type;
};

struct ResetEvent {
	bool direct;
};

struct ShutdownEvent {};

struct PauseEvent {
	bool paused;
};

struct FastForwardEvent {
	bool fastforward;
};

struct StopThreadEvent {};

struct ErrorEvent {
	QString msg;
};

struct RenderEvent {};

struct EndFrameEvent {
	double frametime;
};

using Event = std::variant<EmptyEvent, LoadROMEvent, LoadSystemFileEvent,
		ResetEvent, ShutdownEvent, PauseEvent, FastForwardEvent,
		StopThreadEvent>;

using MainEvent = std::variant<EmptyEvent, ErrorEvent, RenderEvent,
		EndFrameEvent>;

#endif
