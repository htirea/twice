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

class EmulatorThread : public QThread {
	Q_OBJECT
      public:
	EmulatorThread(SharedBuffers *bufs, QObject *parent);
	~EmulatorThread();
	void push_event(const Event& ev);

      protected:
	void run() override;

      private:
	void process_events();
	void process_event(const EmptyEvent& ev);
	void process_event(const LoadFileEvent& ev);
	void process_event(const UnloadFileEvent& ev);
	void process_event(const SaveTypeEvent& ev);
	void process_event(const StopThreadEvent& ev);
	void process_event(const ResetEvent& ev);
	void process_event(const ShutdownEvent& ev);
	void process_event(const PauseEvent& ev);
	void process_event(const FastForwardEvent& ev);
	void process_event(const ButtonEvent& ev);

      signals:
	void send_main_event(const MainEvent& ev);

      private:
	bool running{};
	bool paused{};
	bool throttle{};
	bool shutdown{};
	std::unique_ptr<twice::nds_machine> nds;
	twice::threaded_queue<Event> event_q;
	SharedBuffers *bufs{};
};

#endif
