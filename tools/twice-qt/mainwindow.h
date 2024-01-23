#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <memory>

#include <QAction>
#include <QActionGroup>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QAudioSource>
#include <QCommandLineParser>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
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

#include "libtwice/util/stopwatch.h"
#include "libtwice/util/threaded_queue.h"
#include "libtwice/util/triple_buffer.h"

#include "audio_buffer_device.h"
#include "display_widget.h"
#include "emulator_events.h"
#include "emulator_thread.h"

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
	void shutdown_nds();
	void pause_nds(bool pause);
	void fast_forward_nds(bool fast_forward);
	void set_orientation(int orientation);
	void set_screen_size(int scale);
	void auto_resize_window();
	void create_actions();
	void create_menus();
	void update_audio_outputs();
	void set_audio_output_device(std::size_t idx);
	void set_audio_output_device(const QString& name);

	template <typename Func2>
	std::unique_ptr<QAction> create_action(const QString& text,
			const QString& tip, Func2&& slot,
			bool checkable = false)
	{
		auto action = std::make_unique<QAction>(text, this);
		action->setStatusTip(tip);
		action->setCheckable(checkable);

		if (checkable) {
			connect(action.get(), &QAction::toggled, this, slot);
		} else {
			connect(action.get(), &QAction::triggered, this, slot);
		}

		return action;
	}

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
	std::vector<QAudioDevice> audio_devices;
	std::vector<QAction *> audio_output_acts;
	stopwatch audio_stopwatch;

	QAudioFormat audio_format;
	QAudioFormat mic_audio_format;
	QMediaDevices media_devices;
	QSettings *settings{};
	QCommandLineParser *parser{};
	DisplayWidget *display{};
	QAudioSink *audio_sink{};
	QIODevice *audio_out_buffer{};
	QAudioSource *audio_source{};
	AudioBufferDevice *mic_io_device{};
	EmulatorThread *emu_thread{};
	QMenu *file_menu{};
	QMenu *emu_menu{};
	QMenu *video_menu{};
	QMenu *screen_size_menu{};
	QMenu *orientation_menu{};
	QMenu *texture_filter_menu{};
	QMenu *audio_menu{};
	QMenu *audio_output_menu{};
	std::unique_ptr<QAction> load_rom_act;
	std::unique_ptr<QAction> load_system_files_act;
	std::unique_ptr<QAction> reset_direct;
	std::unique_ptr<QAction> reset_firmware_act;
	std::unique_ptr<QAction> shutdown_act;
	std::unique_ptr<QAction> pause_act;
	std::unique_ptr<QAction> fast_forward_act;
	std::unique_ptr<QAction> orientation_acts[4];
	std::unique_ptr<QActionGroup> orientation_group;
	std::unique_ptr<QActionGroup> texture_filter_group;
	std::unique_ptr<QAction> filter_nearest_act;
	std::unique_ptr<QAction> filter_linear_act;
	std::unique_ptr<QActionGroup> audio_output_group;
	std::unique_ptr<QActionGroup> screen_size_group;
	std::unique_ptr<QAction> screen_size_acts[4];
	std::unique_ptr<QAction> auto_resize_act;
	std::unique_ptr<QAction> stretched_act;

      public slots:
	void frame_ended(double frametime);
	void render_frame();
	void push_audio(size_t len);
	void show_error_msg(QString msg);

      private slots:
	void load_rom();
	void load_system_files();
	void reboot_nds(bool direct);
};

} // namespace twice

#endif
