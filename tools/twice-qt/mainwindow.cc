#include "mainwindow.h"

namespace twice {

enum {
	CMD_BUTTON_A,
	CMD_BUTTON_B,
	CMD_BUTTON_SELECT,
	CMD_BUTTON_START,
	CMD_BUTTON_RIGHT,
	CMD_BUTTON_LEFT,
	CMD_BUTTON_UP,
	CMD_BUTTON_DOWN,
	CMD_BUTTON_R,
	CMD_BUTTON_L,
	CMD_BUTTON_X,
	CMD_BUTTON_Y,
};

MainWindow::MainWindow(QSettings *settings, QWidget *parent)
	: QMainWindow(parent), settings(settings), tbuffer{ {} }, abuffer{ {} }
{
	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	display = new DisplayWidget(&tbuffer, &event_q);
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

	emu_thread = new EmulatorThread(
			settings, display, &tbuffer, &abuffer, &event_q);
	connect(emu_thread, &EmulatorThread::end_frame, this,
			&MainWindow::frame_ended);
	connect(emu_thread, &EmulatorThread::render_frame, this,
			&MainWindow::render_frame);
	connect(emu_thread, &EmulatorThread::push_audio, this,
			&MainWindow::push_audio);

	set_display_size(512, 768);
	setAcceptDrops(true);

	initialize_commands();
	set_default_keybinds();
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
MainWindow::initialize_commands()
{
	for (int cmd = CMD_BUTTON_A; cmd <= CMD_BUTTON_Y; cmd++) {
		cmd_map[cmd] = ([=](intptr_t arg) {
			set_nds_button_state((nds_button)cmd, arg == 1);
		});
	}
}

void
MainWindow::set_default_keybinds()
{
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_X)] = CMD_BUTTON_A;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_Z)] = CMD_BUTTON_B;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_2)] =
			CMD_BUTTON_SELECT;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_1)] =
			CMD_BUTTON_START;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_Right)] =
			CMD_BUTTON_RIGHT;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_Left)] =
			CMD_BUTTON_LEFT;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_Up)] = CMD_BUTTON_UP;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_Down)] =
			CMD_BUTTON_DOWN;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_W)] = CMD_BUTTON_R;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_Q)] = CMD_BUTTON_L;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_S)] = CMD_BUTTON_X;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_A)] = CMD_BUTTON_Y;
}

void
MainWindow::set_nds_button_state(nds_button button, bool down)
{
	event_q.push(ButtonEvent{ .which = button, .down = down });
}

void
MainWindow::frame_ended()
{
}

void
MainWindow::render_frame()
{
	display->render();
}

void
MainWindow::push_audio(size_t len)
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
		event_q.push(LoadROMEvent{
				.pathname = url.toLocalFile().toStdString() });
	}

	event_q.push(BootEvent{ .direct = true });
}

void
MainWindow::keyPressEvent(QKeyEvent *e)
{
	if (e->isAutoRepeat()) {
		e->ignore();
		return;
	}

	auto cmd = keybinds.find(e->keyCombination());
	if (cmd != keybinds.end()) {
		cmd_map[(int)cmd.value()](1);
	} else {
		e->ignore();
	}
}

void
MainWindow::keyReleaseEvent(QKeyEvent *e)
{
	if (e->isAutoRepeat()) {
		e->ignore();
		return;
	}

	auto cmd = keybinds.find(e->keyCombination());
	if (cmd != keybinds.end()) {
		cmd_map[cmd.value()](0);
	} else {
		e->ignore();
	}
}

} // namespace twice
