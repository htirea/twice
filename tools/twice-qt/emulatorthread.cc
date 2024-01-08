#include "emulatorthread.h"

namespace twice {

EmulatorThread::EmulatorThread(QSettings *settings, Display *display,
		triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer)
	: settings(settings), display(display), tbuffer(tbuffer)
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
		std::memcpy(tbuffer->get_write_buffer().data(),
				nds->get_framebuffer(), NDS_FB_SZ_BYTES);
		tbuffer->swap_write_buffer();

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
