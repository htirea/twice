#ifndef TWICE_EMULATORTHREAD_H
#define TWICE_EMULATORTHREAD_H

#include <array>
#include <atomic>
#include <mutex>

#include <QSettings>
#include <QThread>

#include "libtwice/nds/machine.h"

#include "display.h"
#include "events.h"
#include "util/threaded_queue.h"
#include "util/triple_buffer.h"

namespace twice {

class EmulatorThread : public QThread {
	Q_OBJECT

      public:
	EmulatorThread(QSettings *settings, Display *display,
			triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer);
	void run() override;
	void wait();

      private:
	void handle_events();
	void handle_event(const DummyEvent& e);
	void handle_event(const QuitEvent& e);
	void handle_event(const LoadFileEvent& e);
	void handle_event(const LoadROMEvent& e);
	void handle_event(const SetSavetypeEvent& e);
	void handle_event(const BootEvent& e);
	void handle_event(const PauseEvent& e);
	void handle_event(const ResumeEvent& e);
	void handle_event(const StopEvent& e);
	void handle_event(const ResetEvent& e);
	void handle_event(const RotateEvent& e);
	void handle_event(const ButtonEvent& e);
	void handle_event(const TouchEvent& e);
	void handle_event(const UpdateRTCEvent& e);

      public:
	threaded_queue<Event> event_q;

      private:
	bool throttle{};

	enum {
		STOPPED,
		PAUSED,
		RUNNING,
	} state;

	bool quit{};

	std::unique_ptr<nds_machine> nds;
	QSettings *settings{};
	Display *display{};
	triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer{};

      signals:
	void end_frame();
};

} // namespace twice

#endif // TWICE_EMULATORTHREAD_H
