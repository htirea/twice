#include "mainwindow.h"

namespace twice {

MainWindow::MainWindow(QSettings *settings, QWidget *parent)
	: QMainWindow(parent), settings(settings), tbuffer{ {} }, abuffer{ {} }
{
	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	display = new Display(&tbuffer);
	setCentralWidget(display);

	QAudioFormat audio_format;
	audio_format.setSampleRate(32768);
	audio_format.setChannelCount(2);
	audio_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
	audio_format.setSampleFormat(QAudioFormat::Int16);

	QAudioDevice info(QMediaDevices::defaultAudioOutput());
	if (!info.isFormatSupported(audio_format)) {
		throw twice_error("audio format not supported");
	}

	audio_sink = new QAudioSink(audio_format, this);
	audio_sink->setBufferSize(4096);
	audio_out_buffer = audio_sink->start();

	emu_thread = new EmulatorThread(settings, display, &tbuffer, &abuffer);
	connect(emu_thread, &EmulatorThread::end_frame, this,
			&MainWindow::frame_ended);
	connect(emu_thread, &EmulatorThread::queue_audio_signal, this,
			&MainWindow::audio_queued);

	set_display_size(512, 768);
	setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
	emu_thread->wait();
	delete emu_thread;
	delete audio_sink;
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

void
MainWindow::audio_queued(size_t len)
{
	audio_out_buffer->write(
			(const char *)abuffer.get_read_buffer().data(), len);
}

void
MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void
MainWindow::dropEvent(QDropEvent *e)
{
	for (const auto& url : e->mimeData()->urls()) {
		emu_thread->event_q.push(LoadROMEvent{
				.pathname = url.toLocalFile().toStdString() });
	}

	emu_thread->event_q.push(BootEvent{ .direct = true });
}

} // namespace twice
