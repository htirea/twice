#ifndef TWICE_QT_EMULATOR_THREAD_H
#define TWICE_QT_EMULATOR_THREAD_H

#include <array>
#include <atomic>
#include <mutex>

#include <QSettings>
#include <QThread>

#include "libtwice/nds/machine.h"
#include "libtwice/util/frame_timer.h"
#include "libtwice/util/threaded_queue.h"
#include "libtwice/util/triple_buffer.h"

#include "display_widget.h"
#include "emulator_events.h"

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
	void queue_audio(s16 *buffer, size_t len);
	void handle_events();
	void handle_event(const DummyEvent& e);
	void handle_event(const QuitEvent& e);
	void handle_event(const LoadFileEvent& e);
	void handle_event(const LoadROMEvent& e);
	void handle_event(const SetSavetypeEvent& e);
	void handle_event(const PauseEvent& e);
	void handle_event(const ResumeEvent& e);
	void handle_event(const StopEvent& e);
	void handle_event(const SetFastForwardEvent& e);
	void handle_event(const ResetEvent& e);
	void handle_event(const RotateEvent& e);
	void handle_event(const ButtonEvent& e);
	void handle_event(const TouchEvent& e);
	void handle_event(const UpdateRTCEvent& e);

      private:
	bool throttle{};
	bool paused{};
	bool quit{};

	std::unique_ptr<nds_machine> nds;
	QSettings *settings{};
	DisplayWidget *display{};
	triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer{};
	triple_buffer<std::array<s16, 2048>> *abuffer{};
	threaded_queue<Event> *event_q{};

      signals:
	void render_frame();
	void push_audio(size_t len);
	void end_frame(double frametime);
	void show_error_msg(QString msg);
};

} // namespace twice

#endif // TWICE_EMULATORTHREAD_H
