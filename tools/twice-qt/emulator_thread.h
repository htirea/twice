#ifndef TWICE_QT_EMULATOR_THREAD_H
#define TWICE_QT_EMULATOR_THREAD_H

#include <QThread>

#include <filesystem>
#include <memory>
#include <variant>

#include "libtwice/nds/machine.h"
#include "libtwice/util/threaded_queue.h"

#include "buffers.h"
#include "events.h"

class ConfigManager;

class EmulatorThread : public QThread {
	Q_OBJECT
      public:
	EmulatorThread(SharedBuffers *bufs, ConfigManager *cfg,
			QObject *parent);
	~EmulatorThread();
	void push_event(const Event::Event& ev);

      protected:
	void run() override;

      private:
	void process_events();
	void process_event(const Event::EmptyEvent& ev);
	void process_event(const Event::LoadFileEvent& ev);
	void process_event(const Event::UnloadFileEvent& ev);
	void process_event(const Event::SaveTypeEvent& ev);
	void process_event(const Event::StopThreadEvent& ev);
	void process_event(const Event::ResetEvent& ev);
	void process_event(const Event::ShutdownEvent& ev);
	void process_event(const Event::PauseEvent& ev);
	void process_event(const Event::FastForwardEvent& ev);
	void process_event(const Event::ButtonEvent& ev);
	void process_event(const Event::TouchEvent& ev);

      signals:
	void send_main_event(const Event::MainEvent& ev);

      private:
	bool running{};
	bool paused{};
	bool throttle{};
	bool shutdown{};
	std::unique_ptr<twice::nds_machine> nds;
	twice::threaded_queue<Event::Event> event_q;
	SharedBuffers *bufs{};
};

#endif
