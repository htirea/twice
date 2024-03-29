#ifndef TWICE_QT_EMULATOR_THREAD_H
#define TWICE_QT_EMULATOR_THREAD_H

#include <QThread>

#include <atomic>
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
	void process_event(const Event::LoadFile& ev);
	void process_event(const Event::UnloadFile& ev);
	void process_event(const Event::SaveType& ev);
	void process_event(const Event::StopThread& ev);
	void process_event(const Event::Reset& ev);
	void process_event(const Event::Shutdown& ev);
	void process_event(const Event::Restore& ev);
	void process_event(const Event::Pause& ev);
	void process_event(const Event::FastForward& ev);
	void process_event(const Event::Touch& ev);
	void process_event(const Event::Audio& ev);
	void on_shutdown_maybe_changed();
	void on_loaded_files_maybe_changed();
	void on_savetype_maybe_changed();
	void on_nds_instance_maybe_changed();

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

      public:
	std::atomic<unsigned> button_bits;
};

#endif
