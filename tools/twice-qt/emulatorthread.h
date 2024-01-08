#ifndef TWICE_EMULATORTHREAD_H
#define TWICE_EMULATORTHREAD_H

#include <array>
#include <atomic>
#include <mutex>

#include <QSettings>
#include <QThread>

#include "libtwice/nds/machine.h"

#include "display.h"
#include "triple_buffer.h"
#include "frame_timer.h"

namespace twice {

class EmulatorThread : public QThread {
	Q_OBJECT

      public:
	EmulatorThread(QSettings *settings, Display *display,
			triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer);
	void run() override;
	void wait();

      private:
	bool throttle{};

	std::atomic<bool> quit{};
	std::unique_ptr<nds_machine> nds;
	QSettings *settings{};
	Display *display{};
	triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer{};

      signals:
	void end_frame();
};

} // namespace twice

#endif // TWICE_EMULATORTHREAD_H
