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
	CMD_PAUSE,
	CMD_FAST_FORWARD,
};

MainWindow::MainWindow(QSettings *settings, QCommandLineParser *parser,
		QWidget *parent)
	: QMainWindow(parent),
	  tbuffer{ {} },
	  abuffer{ {} },
	  settings(settings),
	  parser(parser)
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
	connect(emu_thread, &EmulatorThread::show_error_msg, this,
			&MainWindow::show_error_msg);

	set_display_size(512, 768);
	setAcceptDrops(true);

	initialize_commands();
	create_actions();
	create_menus();
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
	auto args = parser->positionalArguments();

	if (!args.isEmpty()) {
		event_q.push(LoadROMEvent{
				.pathname = args[0].toStdString() });
		event_q.push(BootEvent{ .direct = true });
	}
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
			set_nds_button_state((nds_button)cmd, arg & 1);
		});
	}

	cmd_map[CMD_PAUSE] = ([=](intptr_t arg) {
		if (arg & 1)
			pause_act->toggle();
	});

	cmd_map[CMD_FAST_FORWARD] = ([=](intptr_t arg) {
		if (arg & 1)
			fast_forward_act->toggle();
	});
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
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_P)] = CMD_PAUSE;
	keybinds[QKeyCombination(Qt::NoModifier, Qt::Key_0)] =
			CMD_FAST_FORWARD;
}

void
MainWindow::set_nds_button_state(nds_button button, bool down)
{
	event_q.push(ButtonEvent{ .which = button, .down = down });
}

void
MainWindow::pause_nds(bool pause)
{
	if (pause) {
		event_q.push(PauseEvent{});
	} else {
		event_q.push(ResumeEvent{});
	}
}

void
MainWindow::fast_forward_nds(bool fast_forward)
{
	event_q.push(SetFastForwardEvent{ .fast_forward = fast_forward });
}

void
MainWindow::create_actions()
{
	load_rom_act = std::make_unique<QAction>(tr("Load ROM"), this);
	load_rom_act->setShortcuts(QKeySequence::Open);
	load_rom_act->setStatusTip(tr("Load a ROM file"));
	connect(load_rom_act.get(), &QAction::triggered, this,
			&MainWindow::load_rom);

	boot_direct_act = std::make_unique<QAction>(tr("Boot ROM"), this);
	boot_direct_act->setStatusTip(tr("Boot the ROM directly"));
	connect(boot_direct_act.get(), &QAction::triggered, this,
			([=]() { boot_nds(true); }));

	boot_firmware_act =
			std::make_unique<QAction>(tr("Boot firmware"), this);
	boot_firmware_act->setStatusTip(tr("Boot the firmware"));
	connect(boot_firmware_act.get(), &QAction::triggered, this,
			([=]() { boot_nds(false); }));

	pause_act = std::make_unique<QAction>(tr("Pause"), this);
	pause_act->setStatusTip(tr("Pause emulation"));
	pause_act->setCheckable(true);
	connect(pause_act.get(), &QAction::toggled, this,
			&MainWindow::pause_nds);

	fast_forward_act = std::make_unique<QAction>(tr("Fast forward"), this);
	fast_forward_act->setStatusTip(tr("Fast forward emulation"));
	fast_forward_act->setCheckable(true);
	connect(fast_forward_act.get(), &QAction::toggled, this,
			&MainWindow::fast_forward_nds);
}

void
MainWindow::create_menus()
{
	file_menu = menuBar()->addMenu(tr("File"));
	file_menu->addAction(load_rom_act.get());

	emu_menu = menuBar()->addMenu(tr("Emulation"));
	emu_menu->addAction(boot_direct_act.get());
	emu_menu->addAction(boot_firmware_act.get());
	emu_menu->addAction(pause_act.get());
	emu_menu->addAction(fast_forward_act.get());
}

void
MainWindow::frame_ended(double frametime)
{
	double fps = 1 / frametime;
	auto title = QString("Twice [%1 fps | %2 ms]")
	                             .arg(fps, 0, 'f', 2)
	                             .arg(frametime * 1000, 0, 'f', 2);
	window()->setWindowTitle(title);
}

void
MainWindow::render_frame()
{
	display->render();
}

void
MainWindow::push_audio(size_t len)
{
	static double acc = 0;
	double early_threshold = -NDS_AVG_SAMPLES_PER_FRAME;
	double late_threshold = NDS_AVG_SAMPLES_PER_FRAME;

	auto elapsed = audio_stopwatch.start();
	double samples = NDS_AVG_SAMPLES_PER_FRAME;
	double samples_max =
			NDS_AUDIO_SAMPLE_RATE * to_seconds<double>(elapsed);
	acc += samples - samples_max;

	if (acc < early_threshold) {
		acc = early_threshold;
	}

	if (acc > late_threshold) {
		acc = 0;
		return;
	}

	if (audio_sink->state() == QAudio::IdleState) {
		audio_out_buffer = audio_sink->start();
	}

	audio_out_buffer->write(
			(const char *)abuffer.get_read_buffer().data(), len);
}

void
MainWindow::show_error_msg(QString msg)
{
	QMessageBox msgbox;
	msgbox.setText(msg);
	msgbox.exec();
}

void
MainWindow::load_rom()
{
	auto pathname = QFileDialog::getOpenFileName(
			this, tr("Load NDS ROM"), "", tr("ROM Files (*.nds)"));
	if (!pathname.isEmpty()) {
		event_q.push(LoadROMEvent{
				.pathname = pathname.toStdString() });
	}
}

void
MainWindow::boot_nds(bool direct)
{
	event_q.push(BootEvent{ .direct = direct });
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
