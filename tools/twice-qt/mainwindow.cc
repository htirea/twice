#include "mainwindow.h"

#include "audio_io.h"
#include "config_manager.h"
#include "display_widget.h"
#include "emulator_thread.h"
#include "input_manager.h"
#include "settings/input_settings.h"
#include "settings_dialog.h"

#include "libtwice/nds/defs.h"
#include "libtwice/nds/display.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#include <iostream>

using namespace twice;
using namespace MainWindowAction;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	cfg = new ConfigManager(this);
	connect(cfg, &ConfigManager::key_set, this,
			&MainWindow::config_var_set);

	display = new DisplayWidget(&bufs.vb, cfg, this);
	setCentralWidget(display);

	audio_out = new AudioIO(&bufs, this);
	input = new InputManager(cfg, this);

	settings_dialog = new SettingsDialog(this);
	auto input_settings = new InputSettings(cfg, nullptr);
	settings_dialog->add_page(tr("Input"), input_settings);

	emu_thread = new EmulatorThread(&bufs, cfg, this);
	connect(emu_thread, &EmulatorThread::finished, emu_thread,
			&QObject::deleteLater);
	connect(emu_thread, &EmulatorThread::send_main_event, this,
			&MainWindow::process_main_event);

	auto timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &MainWindow::update_title);
	timer->start(1000);

	init_actions();
	init_menus();
	init_default_values();
	cfg->emit_all_signals();

	emu_thread->start();
}

MainWindow::~MainWindow()
{
	emu_thread->push_event(Event::StopThread());
	emu_thread->wait();

	QSettings settings;
	auto window_size = size();
	settings.setValue("window_width", window_size.width());
	settings.setValue("window_height", window_size.height());
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
MainWindow::keyPressEvent(QKeyEvent *ev)
{
	if (ev->isAutoRepeat()) {
		ev->ignore();
		return;
	}

	auto button = input->get_nds_button(ev->key());
	if (button == nds_button::NONE) {
		ev->ignore();
		return;
	}

	emu_thread->push_event(Event::Button{ button, true });
}

void
MainWindow::keyReleaseEvent(QKeyEvent *ev)
{
	if (ev->isAutoRepeat()) {
		ev->ignore();
		return;
	}

	auto button = input->get_nds_button(ev->key());
	if (button == nds_button::NONE) {
		ev->ignore();
		return;
	}

	emu_thread->push_event(Event::Button{ button, false });
}

void
MainWindow::mousePressEvent(QMouseEvent *ev)
{
	auto button = ev->button();
	if (button != Qt::LeftButton && button != Qt::RightButton) {
		ev->ignore();
		return;
	}

	if (auto coords = get_nds_coords(ev)) {
		emu_thread->push_event(Event::Touch{ coords->first,
				coords->second, true,
				button == Qt::RightButton });
	}
}

void
MainWindow::mouseMoveEvent(QMouseEvent *ev)
{
	auto buttons = QGuiApplication::mouseButtons();
	if (!(buttons & Qt::LeftButton)) {
		ev->ignore();
		return;
	}

	if (auto coords = get_nds_coords(ev)) {
		emu_thread->push_event(Event::Touch{
				coords->first, coords->second, true, false });
	}
}

void
MainWindow::mouseReleaseEvent(QMouseEvent *ev)
{
	auto button = ev->button();
	if (button != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	emu_thread->push_event(Event::Touch{ 0, 0, false, false });
}

void
MainWindow::init_default_values()
{
	actions[SET_SAVETYPE_AUTO]->trigger();

	QSettings settings;
	int window_width = settings.value("window_width").toInt();
	int window_height = settings.value("window_height").toInt();
	if (window_width < (int)NDS_FB_W || window_height < (int)NDS_FB_W) {
		set_display_size(NDS_FB_W, NDS_FB_H);
	} else {
		resize(window_width, window_height);
	}

	emu_thread->push_event(Event::Shutdown());
	emu_thread->push_event(Event::UnloadFile{ nds_file::CART_ROM });
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
MainWindow::auto_resize_display()
{
	auto size = display->size();
	double w = size.width();
	double h = size.height();
	double ratio = (double)w / h;
	double t = display->orientation & 1 ? NDS_FB_ASPECT_RATIO_RECIP
	                                    : NDS_FB_ASPECT_RATIO;

	if (ratio < t) {
		h = w / t;
	} else if (ratio > t) {
		w = h * t;
	}

	set_display_size(w, h);
}

std::optional<std::pair<int, int>>
MainWindow::get_nds_coords(QMouseEvent *ev)
{
	auto pos = display->mapFromParent(ev->position());
	auto size = display->size();

	double w = size.width();
	double h = size.height();
	double x = pos.x();
	double y = pos.y();
	return window_coords_to_screen_coords(w, h, x, y,
			display->lock_aspect_ratio, display->orientation, 0);
}

void
MainWindow::process_event(const Event::Error& e)
{
	QMessageBox::critical(nullptr, tr("Error"), e.msg);
}

void
MainWindow::process_event(const Event::Shutdown& ev)
{
	using enum nds_file;
	shutdown = ev.shutdown;

	bool cart = loaded_files & (int)CART_ROM;
	bool save = loaded_files & (int)SAVE;

	actions[LOAD_SYSTEM_FILES]->setEnabled(shutdown);
	actions[INSERT_CART]->setEnabled(shutdown && !cart);
	actions[EJECT_CART]->setEnabled(shutdown && cart);
	actions[LOAD_SAVE_FILE]->setEnabled(shutdown);
	actions[UNLOAD_SAVE_FILE]->setEnabled(shutdown && save);
	for (int id = SET_SAVETYPE_AUTO; id != SET_SAVETYPE_NAND; id++) {
		actions[id]->setEnabled(shutdown);
	}
	actions[SHUTDOWN]->setEnabled(!shutdown);
	actions[TOGGLE_PAUSE]->setEnabled(!shutdown);
	actions[TOGGLE_FASTFORWARD]->setEnabled(!shutdown);
}

void
MainWindow::process_event(const Event::File& ev)
{
	using enum nds_file;
	loaded_files = ev.loaded_files;

	bool arm9_bios = loaded_files & (int)ARM9_BIOS;
	bool arm7_bios = loaded_files & (int)ARM7_BIOS;
	bool firmware = loaded_files & (int)FIRMWARE;
	bool sys_files = arm9_bios && arm7_bios && firmware;
	bool cart = loaded_files & (int)CART_ROM;
	bool save = loaded_files & (int)SAVE;

	actions[OPEN_ROM]->setEnabled(sys_files);
	actions[EJECT_CART]->setEnabled(shutdown && cart);
	actions[RESET_TO_ROM]->setEnabled(sys_files && cart);
	actions[UNLOAD_SAVE_FILE]->setEnabled(shutdown && save);
	actions[RESET_TO_FIRMWARE]->setEnabled(sys_files);
}

void
MainWindow::process_event(const Event::SaveType& ev)
{
	auto id = SET_SAVETYPE_AUTO + (ev.type + 1);
	actions[id]->setChecked(true);
}

void
MainWindow::process_event(const Event::EndFrame& ev)
{
	display->update();
	double a = 0.9;
	avg_frametime = a * avg_frametime + (1 - a) * ev.frametime;
	avg_usage[0] = a * avg_usage[0] + (1 - a) * ev.cpu_usage.first;
	avg_usage[1] = a * avg_usage[1] + (1 - a) * ev.cpu_usage.second;
	avg_usage[2] = a * avg_usage[2] + (1 - a) * ev.dma_usage.first;
	avg_usage[3] = a * avg_usage[3] + (1 - a) * ev.dma_usage.second;
}

bool
MainWindow::confirm_shutdown()
{
	auto result = QMessageBox::question(this, tr("Shutdown"),
			tr("Do you want to shutdown the virtual machine?"));
	return result == QMessageBox::Yes;
}

void
MainWindow::process_main_event(const Event::MainEvent& ev)
{
	std::visit([this](const auto& ev) { process_event(ev); }, ev);
}

void
MainWindow::open_rom()
{
	auto pathname = QFileDialog::getOpenFileName(
			this, tr("Open ROM"), "", tr("ROM Files (*.nds)"));
	if (!pathname.isEmpty()) {
		emu_thread->push_event(Event::Shutdown());
		emu_thread->push_event(Event::LoadFile{
				pathname, nds_file::CART_ROM });
		emu_thread->push_event(Event::Reset{ true });
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
			cfg->set(ConfigVariable::ARM9_BIOS_PATH, pathname);
		} else if (info.fileName() == "bios7.bin" ||
				info.size() == 16384) {
			type = nds_file::ARM7_BIOS;
			cfg->set(ConfigVariable::ARM7_BIOS_PATH, pathname);
		} else if (info.fileName() == "firmware.bin" ||
				info.size() == 262144) {
			type = nds_file::FIRMWARE;
			cfg->set(ConfigVariable::FIRMWARE_PATH, pathname);
		}

		emu_thread->push_event(Event::LoadFile{ pathname, type });
	}
}

void
MainWindow::load_save_file()
{
	auto pathname = QFileDialog::getOpenFileName(this,
			tr("Load save file"), "", tr("SAV Files (*.sav)"));
	if (!pathname.isEmpty()) {
		emu_thread->push_event(
				Event::LoadFile{ pathname, nds_file::SAVE });
	}
}

void
MainWindow::insert_cart()
{
	auto pathname = QFileDialog::getOpenFileName(this,
			tr("Insert cartridge"), "", tr("ROM Files (*.nds)"));
	if (!pathname.isEmpty()) {
		emu_thread->push_event(Event::LoadFile{
				pathname, nds_file::CART_ROM });
	}
}

void
MainWindow::eject_cart()
{
	emu_thread->push_event(Event::UnloadFile{ nds_file::CART_ROM });
}

void
MainWindow::unload_save_file()
{
	emu_thread->push_event(Event::UnloadFile{ nds_file::SAVE });
}

void
MainWindow::set_savetype(int type)
{
	emu_thread->push_event(Event::SaveType{ (nds_savetype)type });
}

void
MainWindow::set_scale(int scale)
{
	int w = NDS_FB_W * scale;
	int h = NDS_FB_H * scale;

	if (display->orientation & 1) {
		std::swap(w, h);
	}

	set_display_size(w, h);
}

void
MainWindow::set_orientation(int orientation)
{
	if (orientation < 0) {
		orientation = (display->orientation + orientation) & 3;
	}

	cfg->set(ConfigVariable::ORIENTATION, orientation);
}

void
MainWindow::reset_to_rom()
{
	reset_emulation(true);
}

void
MainWindow::reset_to_firmware()
{
	reset_emulation(false);
}

void
MainWindow::reset_emulation(bool direct)
{
	emu_thread->push_event(Event::Reset{ direct });
}

void
MainWindow::shutdown_emulation()
{
	if (confirm_shutdown()) {
		emu_thread->push_event(Event::Shutdown());
	}
}

void
MainWindow::toggle_pause(bool checked)
{
	emu_thread->push_event(Event::Pause{ checked });
}

void
MainWindow::toggle_fastforward(bool checked)
{
	emu_thread->push_event(Event::FastForward{ checked });
}

void
MainWindow::toggle_linear_filtering(bool checked)
{
	cfg->set(ConfigVariable::LINEAR_FILTERING, checked);
}

void
MainWindow::toggle_lock_aspect_ratio(bool checked)
{
	cfg->set(ConfigVariable::LOCK_ASPECT_RATIO, checked);
}

void
MainWindow::update_title()
{
	double fps = 1 / avg_frametime;
	auto title = QString("Twice [%1 fps | %2 ms | %3 | %4]")
	                             .arg(fps, 0, 'f', 2)
	                             .arg(avg_frametime * 1000, 0, 'f', 2)
	                             .arg(avg_usage[0], 0, 'f', 2)
	                             .arg(avg_usage[1], 0, 'f', 2);
	window()->setWindowTitle(title);
}

void
MainWindow::open_settings()
{
	settings_dialog->exec();
}

void
MainWindow::config_var_set(int key, const QVariant& v)
{
	if (!v.isValid())
		return;

	switch (key) {
	case ConfigVariable::ORIENTATION:
		actions[ORIENTATION_0 + v.toInt()]->setChecked(true);
		break;
	case ConfigVariable::LOCK_ASPECT_RATIO:
		actions[LOCK_ASPECT_RATIO]->setChecked(v.toBool());
		break;
	case ConfigVariable::LINEAR_FILTERING:
		actions[LINEAR_FILTERING]->setChecked(v.toBool());
		break;
	}
}
