#ifndef TWICE_QT_EMULATOR_THREAD_H
#define TWICE_QT_EMULATOR_THREAD_H

#include <array>
#include <atomic>
#include <mutex>

#include <QSettings>
#include <QThread>

#include "libtwice/nds/machine.h"

#include "display_widget.h"
#include "emulator_events.h"

#include "util/frame_timer.h"
#include "util/threaded_queue.h"
#include "util/triple_buffer.h"

namespace twice {

class EmulatorThread : public QThread {
	Q_OBJECT

      public:
	EmulatorThread(QSettings *settings, DisplayWidget *display,
			triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer,
			triple_buffer<std::array<s16, 2048>> *abuffer,
			threaded_queue<Event> *event_q);
	void run() override;
	void wait();

      private:
	void queue_audio(s16 *buffer, size_t len,
			frame_timer::duration last_period);
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
	DisplayWidget *display{};
	triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer{};
	triple_buffer<std::array<s16, 2048>> *abuffer{};
	threaded_queue<Event> *event_q{};

      signals:
	void queue_audio_signal(size_t len);
	void end_frame();
};

} // namespace twice

#endif // TWICE_EMULATORTHREAD_H
