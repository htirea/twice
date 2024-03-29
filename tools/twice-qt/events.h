#ifndef TWICE_QT_EVENTS_H
#define TWICE_QT_EVENTS_H

#include "libtwice/nds/game_db.h"
#include "libtwice/nds/machine.h"

#include <QString>

#include <filesystem>

namespace Event {

struct LoadFile {
	QString pathname;
	twice::nds_file type;
};

struct UnloadFile {
	twice::nds_file type;
};

struct SaveType {
	twice::nds_savetype type;
};

struct Reset {
	bool direct;
};

struct Shutdown {
	bool shutdown;
};

struct Restore {
	bool restore;
};

struct Pause {
	bool paused;
};

struct FastForward {
	bool fastforward;
};

struct Touch {
	int x;
	int y;
	bool down;
	bool quicktap;
};

struct Audio {
	bool interpolate_audio;
};

struct StopThread {};

struct Error {
	QString msg;
};

struct File {
	int loaded_files;
};

struct EndFrame {
	double frametime;
	std::pair<double, double> cpu_usage;
	std::pair<double, double> dma_usage;
};

using Event = std::variant<LoadFile, UnloadFile, SaveType, Reset, Shutdown,
		Restore, Pause, FastForward, Touch, Audio, StopThread>;

using MainEvent = std::variant<Error, Shutdown, Restore, File, SaveType,
		EndFrame>;

} // namespace Event

#endif
