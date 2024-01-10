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
	CMD_ROTATE_RIGHT,
	CMD_ROTATE_LEFT,
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

	initialize_commands();
	create_actions();
	create_menus();
	set_default_keybinds();

	set_display_size(512, 768);
	setAcceptDrops(true);
	orientation_acts[0]->setChecked(true);
	filter_linear_act->setChecked(true);
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
		event_q.push(ResetEvent{ .direct = true });
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

	cmd_map[CMD_ROTATE_RIGHT] = ([=](intptr_t arg) {
		if (arg & 1)
			orientation_acts[(display->orientation + 1) & 3]
					->setChecked(true);
	});

	cmd_map[CMD_ROTATE_LEFT] = ([=](intptr_t arg) {
		if (arg & 1)
			orientation_acts[(display->orientation - 1) & 3]
					->setChecked(true);
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
	keybinds[QKeyCombination(Qt::CTRL, Qt::Key_Right)] = CMD_ROTATE_RIGHT;
	keybinds[QKeyCombination(Qt::CTRL, Qt::Key_Left)] = CMD_ROTATE_LEFT;
}

void
MainWindow::set_nds_button_state(nds_button button, bool down)
{
	event_q.push(ButtonEvent{ .which = button, .down = down });
}

void
MainWindow::shutdown_nds()
{
	event_q.push(StopEvent{});
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
MainWindow::set_orientation(int orientation)
{
	display->orientation = orientation;
}

void
MainWindow::create_actions()
{
	load_rom_act = create_action(tr("Load ROM"), tr("Load a ROM file"),
			&MainWindow::load_rom);
	load_rom_act->setShortcuts(QKeySequence::Open);

	load_system_files_act = create_action(tr("Load system files"),
			tr("Load NDS system files"),
			&MainWindow::load_system_files);

	reset_direct = create_action(tr("Reset to ROM"),
			tr("Reset to the ROM directly"),
			([=]() { reboot_nds(true); }));

	reset_firmware_act = create_action(tr("Reset to firmware"),
			tr("Reset to the firmware"),
			([=]() { reboot_nds(false); }));

	shutdown_act = create_action(tr("Shut down"),
			tr("Shut down the NDS machine"),
			([=]() { shutdown_nds(); }));

	pause_act = create_action(tr("Pause"), tr("Pause emulation"),
			&MainWindow::pause_nds, true);

	fast_forward_act = create_action(tr("Fast forward"),
			tr("Fast forward emulation"),
			&MainWindow::fast_forward_nds, true);

	orientation_group = std::make_unique<QActionGroup>(this);
	for (int i = 0; i < 4; i++) {
		int degrees = i * 90;
		orientation_acts[i] = create_action(tr("%1°").arg(degrees),
				tr("Rotate the screen %1 degrees")
						.arg(degrees),
				([=](bool checked) {
					if (checked)
						set_orientation(i);
				}),
				true);
		orientation_group->addAction(orientation_acts[i].get());
	}

	texture_filter_group = std::make_unique<QActionGroup>(this);
	filter_nearest_act = create_action(tr("Nearest"),
			tr("Nearest neighbour filtering"), ([=](bool checked) {
				if (checked)
					display->linear_filtering = false;
			}),
			true);
	texture_filter_group->addAction(filter_nearest_act.get());
	filter_linear_act = create_action(tr("Linear"),
			tr("Bilinear filtering"), ([=](bool checked) {
				if (checked)
					display->linear_filtering = true;
			}),
			true);
	texture_filter_group->addAction(filter_linear_act.get());
}

void
MainWindow::create_menus()
{
	file_menu = menuBar()->addMenu(tr("File"));
	file_menu->addAction(load_rom_act.get());
	file_menu->addAction(load_system_files_act.get());

	emu_menu = menuBar()->addMenu(tr("Emulation"));
	emu_menu->addAction(reset_direct.get());
	emu_menu->addAction(reset_firmware_act.get());
	emu_menu->addAction(shutdown_act.get());
	emu_menu->addSeparator();
	emu_menu->addAction(pause_act.get());
	emu_menu->addAction(fast_forward_act.get());

	video_menu = menuBar()->addMenu(tr("Video"));
	orientation_menu = video_menu->addMenu(tr("Orientation"));
	for (int i = 0; i < 4; i++) {
		orientation_menu->addAction(orientation_acts[i].get());
	}
	texture_filter_menu = video_menu->addMenu(tr("Filter mode"));
	texture_filter_menu->addAction(filter_nearest_act.get());
	texture_filter_menu->addAction(filter_linear_act.get());
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
MainWindow::load_system_files()
{
	auto pathnames = QFileDialog::getOpenFileNames(this,
			tr("Select NDS system files"), "",
			tr("Binary files (*.bin);;All files (*)"));

	for (const auto& path : pathnames) {
		QFileInfo info(path);

		if (info.fileName() == "bios9.bin" || info.size() == 4096) {
			event_q.push(LoadFileEvent{
					.type = LoadFileEvent::ARM9_BIOS,
					.pathname = path.toStdString() });
			settings->setValue("arm9_bios_path", path);
		} else if (info.fileName() == "bios7.bin" ||
				info.size() == 16384) {
			event_q.push(LoadFileEvent{
					.type = LoadFileEvent::ARM7_BIOS,
					.pathname = path.toStdString() });
			settings->setValue("arm7_bios_path", path);
		} else if (info.fileName() == "firmware.bin" ||
				info.size() == 262144) {
			event_q.push(LoadFileEvent{
					.type = LoadFileEvent::FIRMWARE,
					.pathname = path.toStdString() });
			settings->setValue("firmware_path", path);
		} else {
			QMessageBox msgbox;
			msgbox.setText(tr("Unknown system file type: %1")
							.arg(path));
			msgbox.exec();
		}
	}
}

void
MainWindow::reboot_nds(bool direct)
{
	event_q.push(ResetEvent{ .direct = direct });
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
