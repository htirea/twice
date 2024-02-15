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

struct StopThreadEvent {};

struct ErrorEvent {
	QString msg;
};

using Event = std::variant<EmptyEvent, LoadROMEvent, LoadSystemFileEvent,
		StopThreadEvent>;

using MainEvent = std::variant<EmptyEvent, ErrorEvent>;

#endif
