#ifndef TWICE_EMULATORTHREAD_H
#define TWICE_EMULATORTHREAD_H

#include <array>
#include <atomic>
#include <mutex>

#include <QSettings>
#include <QThread>

#include "libtwice/nds/machine.h"

#include "display.h"

namespace twice {

class EmulatorThread : public QThread {
	Q_OBJECT

      public:
	EmulatorThread(QSettings *settings, Display *display);
	void run() override;
	void wait();

      private:
	std::atomic<bool> quit{};
	std::unique_ptr<nds_machine> nds;
	QSettings *settings{};
	Display *display{};

      signals:
	void end_frame();
};

} // namespace twice

#endif // TWICE_EMULATORTHREAD_H
