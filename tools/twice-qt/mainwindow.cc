#include "mainwindow.h"

namespace twice {

MainWindow::MainWindow(QSettings *settings, QWidget *parent)
	: QMainWindow(parent), settings(settings), tbuffer{ {} }
{
	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	display = new Display(&tbuffer);
	setCentralWidget(display);

	emu_thread = new EmulatorThread(settings, display, &tbuffer);
	connect(emu_thread, &EmulatorThread::end_frame, this,
			&MainWindow::frame_ended);

	set_display_size(512, 768);
}

MainWindow::~MainWindow()
{
	emu_thread->wait();
	delete emu_thread;
}

void
MainWindow::start_emulator_thread()
{
	emu_thread->start();
}

void
MainWindow::set_display_size(int w, int h)
{
	resize(w, h);
}

void
MainWindow::frame_ended()
{
	display->render();
}

} // namespace twice
