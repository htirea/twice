#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <memory>

#include <QAction>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QCommandLineParser>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHash>
#include <QLayout>
#include <QMainWindow>
#include <QMediaDevices>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QStandardPaths>

#include "display_widget.h"
#include "emulator_events.h"
#include "emulator_thread.h"
#include "util/stopwatch.h"
#include "util/threaded_queue.h"
#include "util/triple_buffer.h"

namespace twice {

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QSettings *settings, QCommandLineParser *parser,
			QWidget *parent = nullptr);
	~MainWindow();
	void start_emulator_thread();

      private:
	void set_display_size(int w, int h);
	void initialize_commands();
	void set_default_keybinds();
	void set_nds_button_state(nds_button button, bool down);
	void pause_nds(bool pause);
	void fast_forward_nds(bool fast_forward);
	void set_orientation(int orientation);
	void create_actions();
	void create_menus();

      protected:
	void dragEnterEvent(QDragEnterEvent *) override;
	void dropEvent(QDropEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;
	void keyReleaseEvent(QKeyEvent *e) override;

      private:
	threaded_queue<Event> event_q;
	triple_buffer<std::array<u32, NDS_FB_SZ>> tbuffer;
	triple_buffer<std::array<s16, 2048>> abuffer;
	QHash<QKeyCombination, int> keybinds{};
	std::unordered_map<int, std::function<void(intptr_t arg)>> cmd_map;
	stopwatch audio_stopwatch;

	QSettings *settings{};
	QCommandLineParser *parser{};
	DisplayWidget *display{};
	QAudioSink *audio_sink{};
	QIODevice *audio_out_buffer{};
	EmulatorThread *emu_thread{};
	QMenu *file_menu{};
	QMenu *emu_menu{};
	std::unique_ptr<QAction> load_rom_act;
	std::unique_ptr<QAction> boot_direct_act;
	std::unique_ptr<QAction> boot_firmware_act;
	std::unique_ptr<QAction> pause_act;
	std::unique_ptr<QAction> fast_forward_act;

      public slots:
	void frame_ended(double frametime);
	void render_frame();
	void push_audio(size_t len);
	void show_error_msg(QString msg);

      private slots:
	void load_rom();
	void boot_nds(bool direct);
};

} // namespace twice

#endif
