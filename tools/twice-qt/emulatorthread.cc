#include "emulatorthread.h"

namespace twice {

EmulatorThread::EmulatorThread(QSettings *settings, Display *display)
	: settings(settings), display(display)
{
	nds_config config;
	/* TODO: fix file path handling */
	config.data_dir = settings->value("data_dir").toString().toStdString();

	nds = std::make_unique<nds_machine>(config);
	nds->boot(false);
}

void
EmulatorThread::run()
{
	while (!quit) {
		nds->run_frame();
		{
			std::lock_guard lock(display->fb_mtx);
			std::memcpy(display->fb.data(), nds->get_framebuffer(),
					NDS_FB_SZ_BYTES);
		}

		emit end_frame();
	}
}

void
EmulatorThread::wait()
{
	quit = true;
	QThread::wait();
}

} // namespace twice
