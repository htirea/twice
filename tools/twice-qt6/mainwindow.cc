#include "mainwindow.h"

#include "actions.h"
#include "display_widget.h"
#include "emulator_thread.h"

#include "libtwice/nds/defs.h"

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>

#include <iostream>

using namespace twice;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	display = new DisplayWidget(&bufs.vb, this);
	setCentralWidget(display);

	emu_thread = new EmulatorThread(&bufs, this);
	connect(emu_thread, &EmulatorThread::finished, emu_thread,
			&QObject::deleteLater);
	connect(emu_thread, &EmulatorThread::send_main_event, this,
			&MainWindow::process_main_event);

	init_menus();
	init_default_values();

	emu_thread->start();
}

MainWindow::~MainWindow()
{
	emu_thread->push_event(StopThreadEvent());
	emu_thread->wait();
}

void
MainWindow::closeEvent(QCloseEvent *ev)
{
	if (shutdown) {
		ev->accept();
	} else {
		if (confirm_shutdown()) {
			ev->accept();
		} else {
			ev->ignore();
		}
	}
}

void
MainWindow::init_menus()
{
	auto file_menu = menuBar()->addMenu(tr("File"));
	{
		file_menu->addAction(create_action(
				ACTION_OPEN_ROM, this, &MainWindow::open_rom));
		file_menu->addAction(create_action(ACTION_LOAD_SYSTEM_FILES,
				this, &MainWindow::open_system_files));
		file_menu->addSeparator();
		file_menu->addAction(create_action(ACTION_INSERT_CART, this,
				&MainWindow::insert_cart));
		file_menu->addAction(create_action(ACTION_EJECT_CART, this,
				&MainWindow::eject_cart));
		file_menu->addSeparator();
		file_menu->addAction(create_action(ACTION_LOAD_SAVE_FILE, this,
				&MainWindow::load_save_file));
		file_menu->addAction(create_action(ACTION_UNLOAD_SAVE_FILE,
				this, &MainWindow::unload_save_file));
	}
	{
		auto menu = file_menu->addMenu(tr("Save type"));
		auto group = create_action_group(ACTION_GROUP_SAVETYPE, this);
		for (int i = 0; i < SAVETYPE_TOTAL; i++) {
			int id = ACTION_SAVETYPE_AUTO + i;
			menu->addAction(create_action(id, group));
		}

		/* NAND saves not implemented yet */
		get_action(ACTION_SAVETYPE_NAND)->setEnabled(false);

		auto set_type = [=, this](QAction *action) {
			set_savetype(action->data().toInt());
		};
		connect(group, &QActionGroup::triggered, this, set_type);
	}

	auto emu_menu = menuBar()->addMenu(tr("Emulation"));
	{
		auto reset = [=, this]() { reset_emulation(true); };
		auto action = create_action(ACTION_RESET_TO_ROM, this, reset);
		emu_menu->addAction(action);
	}
	{
		auto reset = [=, this]() { reset_emulation(false); };
		auto action = create_action(
				ACTION_RESET_TO_FIRMWARE, this, reset);
		emu_menu->addAction(action);
	}
	{
		emu_menu->addAction(create_action(ACTION_SHUTDOWN, this,
				&MainWindow::shutdown_emulation));
		emu_menu->addSeparator();
		emu_menu->addAction(create_action(ACTION_TOGGLE_PAUSE, this,
				&MainWindow::toggle_pause));
		emu_menu->addAction(create_action(ACTION_TOGGLE_FASTFORWARD,
				this, &MainWindow::toggle_fastforward));
	}

	auto video_menu = menuBar()->addMenu(tr("Video"));
	{
		auto action = create_action(ACTION_AUTO_RESIZE, this,
				&MainWindow::auto_resize_display);
		video_menu->addAction(action);
	}
	{
		auto menu = video_menu->addMenu(tr("Scale"));
		auto group = new QActionGroup(this);
		for (int i = 0; i < 4; i++) {
			int id = ACTION_SCALE_1X + i;
			menu->addAction(create_action(id, group));
		}

		auto set_scale = [=, this](QAction *action) {
			set_display_size(action->data().toInt());
		};
		connect(group, &QActionGroup::triggered, this, set_scale);
	}
	{
		auto menu = video_menu->addMenu(tr("Orientation"));
		menu->addAction(get_action(ACTION_ORIENTATION_0));
		menu->addAction(get_action(ACTION_ORIENTATION_1));
		menu->addAction(get_action(ACTION_ORIENTATION_3));
		menu->addAction(get_action(ACTION_ORIENTATION_2));
	}
	{
		video_menu->addSeparator();
		video_menu->addAction(get_action(ACTION_LINEAR_FILTERING));
		video_menu->addAction(get_action(ACTION_LOCK_ASPECT_RATIO));
	}
}

void
MainWindow::init_default_values()
{
	get_action(ACTION_ORIENTATION_0)->trigger();
	get_action(ACTION_LOCK_ASPECT_RATIO)->trigger();
	get_action(ACTION_SAVETYPE_AUTO)->trigger();
	set_display_size(NDS_FB_W * 2, NDS_FB_H * 2);
	emu_thread->push_event(ShutdownEvent());
	emu_thread->push_event(UnloadFileEvent{ nds_file::CART_ROM });
}

void
MainWindow::set_display_size(int w, int h)
{
	display->setFixedSize(w, h);
	setFixedSize(sizeHint());
	display->setMinimumSize(0, 0);
	display->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	setMinimumSize(0, 0);
	setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void
MainWindow::set_display_size(int scale)
{
	int w = NDS_FB_W * scale;
	int h = NDS_FB_H * scale;

	if (get_orientation() & 1) {
		std::swap(w, h);
	}

	set_display_size(w, h);
}

void
MainWindow::auto_resize_display()
{
	auto size = display->size();
	double w = size.width();
	double h = size.height();
	double ratio = (double)w / h;
	double t = get_orientation() & 1 ? NDS_FB_ASPECT_RATIO_RECIP
	                                 : NDS_FB_ASPECT_RATIO;

	if (ratio < t) {
		h = w / t;
	} else if (ratio > t) {
		w = h * t;
	}

	set_display_size(w, h);
}

void
MainWindow::process_event(const EmptyEvent&)
{
}

void
MainWindow::process_event(const ErrorEvent& e)
{
	QMessageBox::critical(nullptr, tr("Error"), e.msg);
}

void
MainWindow::process_event(const RenderEvent&)
{
	display->update();
}

void
MainWindow::process_event(const ShutdownEvent& ev)
{
	using enum nds_file;
	shutdown = ev.shutdown;

	bool cart = loaded_files & (int)CART_ROM;
	bool save = loaded_files & (int)SAVE;

	get_action(ACTION_LOAD_SYSTEM_FILES)->setEnabled(shutdown);
	get_action(ACTION_INSERT_CART)->setEnabled(shutdown && !cart);
	get_action(ACTION_EJECT_CART)->setEnabled(shutdown && cart);
	get_action(ACTION_LOAD_SAVE_FILE)->setEnabled(shutdown);
	get_action(ACTION_UNLOAD_SAVE_FILE)->setEnabled(shutdown && save);
	for (int id = ACTION_SAVETYPE_AUTO; id != ACTION_SAVETYPE_NAND; id++) {
		get_action(id)->setEnabled(shutdown);
	}
	get_action(ACTION_SHUTDOWN)->setEnabled(!shutdown);
	get_action(ACTION_TOGGLE_PAUSE)->setEnabled(!shutdown);
	get_action(ACTION_TOGGLE_FASTFORWARD)->setEnabled(!shutdown);
}

void
MainWindow::process_event(const FileEvent& ev)
{
	using enum nds_file;
	loaded_files = ev.loaded_files;

	int sys_files_mask = (int)ARM9_BIOS | (int)ARM7_BIOS | (int)FIRMWARE;
	bool sys_files = (loaded_files & sys_files_mask) == sys_files_mask;
	bool cart = loaded_files & (int)CART_ROM;
	bool save = loaded_files & (int)SAVE;

	get_action(ACTION_OPEN_ROM)->setEnabled(sys_files);
	get_action(ACTION_EJECT_CART)->setEnabled(shutdown && cart);
	get_action(ACTION_RESET_TO_ROM)->setEnabled(sys_files && cart);
	get_action(ACTION_UNLOAD_SAVE_FILE)->setEnabled(shutdown && save);
	get_action(ACTION_RESET_TO_FIRMWARE)->setEnabled(sys_files);
}

void
MainWindow::process_event(const SaveTypeEvent& ev)
{
	auto id = ACTION_SAVETYPE_AUTO + (ev.type + 1);
	get_action(id)->setChecked(true);
}

void
MainWindow::process_event(const EndFrameEvent&)
{
}

bool
MainWindow::confirm_shutdown()
{
	auto result = QMessageBox::question(this, tr("Shutdown"),
			tr("Do you want to shutdown the virtual machine?"));
	return result == QMessageBox::Yes;
}

void
MainWindow::process_main_event(const MainEvent& ev)
{
	std::visit([this](const auto& ev) { process_event(ev); }, ev);
}

void
MainWindow::open_rom()
{
	if (!(shutdown || confirm_shutdown()))
		return;

	auto pathname = QFileDialog::getOpenFileName(
			this, tr("Open ROM"), "", tr("ROM Files (*.nds)"));
	if (!pathname.isEmpty()) {
		emu_thread->push_event(ShutdownEvent());
		emu_thread->push_event(
				LoadFileEvent{ pathname, nds_file::CART_ROM });
		emu_thread->push_event(ResetEvent{ true });
	}
}

void
MainWindow::open_system_files()
{
	auto pathnames = QFileDialog::getOpenFileNames(this,
			tr("Select NDS system files"), "",
			tr("Binary files (*.bin);;All files (*)"));

	for (const auto& pathname : pathnames) {
		QFileInfo info(pathname);

		auto type = nds_file::UNKNOWN;

		if (info.fileName() == "bios9.bin" || info.size() == 4096) {
			type = nds_file::ARM9_BIOS;
		} else if (info.fileName() == "bios7.bin" ||
				info.size() == 16384) {
			type = nds_file::ARM7_BIOS;
		} else if (info.fileName() == "firmware.bin" ||
				info.size() == 262144) {
			type = nds_file::FIRMWARE;
		}

		emu_thread->push_event(LoadFileEvent{ pathname, type });
	}
}

void
MainWindow::load_save_file()
{
	auto pathname = QFileDialog::getOpenFileName(this,
			tr("Load save file"), "", tr("SAV Files (*.sav)"));
	if (!pathname.isEmpty()) {
		emu_thread->push_event(
				LoadFileEvent{ pathname, nds_file::SAVE });
	}
}

void
MainWindow::insert_cart()
{
	auto pathname = QFileDialog::getOpenFileName(this,
			tr("Insert cartridge"), "", tr("ROM Files (*.nds)"));
	if (!pathname.isEmpty()) {
		emu_thread->push_event(
				LoadFileEvent{ pathname, nds_file::CART_ROM });
	}
}

void
MainWindow::eject_cart()
{
	emu_thread->push_event(UnloadFileEvent{ nds_file::CART_ROM });
}

void
MainWindow::unload_save_file()
{
	emu_thread->push_event(UnloadFileEvent{ nds_file::SAVE });
}

void
MainWindow::set_savetype(int type)
{
	emu_thread->push_event(SaveTypeEvent{ (nds_savetype)type });
}

void
MainWindow::reset_emulation(bool direct)
{
	emu_thread->push_event(ResetEvent{ direct });
}

void
MainWindow::shutdown_emulation()
{
	if (confirm_shutdown()) {
		emu_thread->push_event(ShutdownEvent());
	}
}

void
MainWindow::toggle_pause(bool checked)
{
	emu_thread->push_event(PauseEvent{ checked });
}

void
MainWindow::toggle_fastforward(bool checked)
{
	emu_thread->push_event(FastForwardEvent{ checked });
}
