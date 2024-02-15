#ifndef TWICE_QT_EVENTS_H
#define TWICE_QT_EVENTS_H

#include "libtwice/nds/game_db.h"
#include "libtwice/nds/machine.h"

#include <QString>

#include <filesystem>

struct EmptyEvent {};

struct LoadFileEvent {
	QString pathname;
	twice::nds_file type;
};

struct UnloadFileEvent {
	twice::nds_file type;
};

struct SaveTypeEvent {
	twice::nds_savetype type;
};

struct ResetEvent {
	bool direct;
};

struct ShutdownEvent {
	bool shutdown;
};

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

struct FileEvent {
	int loaded_files;
};

struct EndFrameEvent {
	double frametime;
};

using Event = std::variant<EmptyEvent, LoadFileEvent, UnloadFileEvent,
		SaveTypeEvent, ResetEvent, ShutdownEvent, PauseEvent,
		FastForwardEvent, StopThreadEvent>;

using MainEvent = std::variant<EmptyEvent, ErrorEvent, RenderEvent,
		ShutdownEvent, FileEvent, SaveTypeEvent, EndFrameEvent>;

#endif
