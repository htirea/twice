#include "mainwindow.h"

namespace twice {

MainWindow::MainWindow(QSettings *settings, QWidget *parent)
	: QMainWindow(parent), settings(settings)
{
	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);
	display = new Display();
	setCentralWidget(display);
	emu_thread = new EmulatorThread(settings, display);

	connect(emu_thread, &EmulatorThread::end_frame, this,
			&MainWindow::frame_ended);
}

MainWindow::~MainWindow()
{
	emu_thread->wait();
	delete emu_thread;
}

void
MainWindow::frame_ended()
{
	display->render();
}

} // namespace twice
