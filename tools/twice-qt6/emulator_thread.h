#ifndef TWICE_QT_EMULATOR_THREAD_H
#define TWICE_QT_EMULATOR_THREAD_H

#include <QThread>

#include <filesystem>
#include <memory>
#include <variant>

#include "libtwice/nds/machine.h"
#include "libtwice/util/threaded_queue.h"

#include "events.h"

class EmulatorThread : public QThread {
	Q_OBJECT
      public:
	EmulatorThread(QObject *parent);
	~EmulatorThread();
	void push_event(const Event& ev);

      protected:
	void run() override;

      private:
	void process_events();
	void process_event(const EmptyEvent& ev);
	void process_event(const LoadROMEvent& ev);
	void process_event(const LoadSystemFileEvent& ev);
	void process_event(const StopThreadEvent& ev);

      signals:
	void send_main_event(const MainEvent& ev);

      private:
	bool running{};
	twice::threaded_queue<Event> event_q;
	std::unique_ptr<twice::nds_machine> nds;
};

#endif
