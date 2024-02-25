#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include "buffers.h"
#include "events.h"
#include "mainwindow_actions.h"

#include <QMainWindow>

class ConfigManager;
class DisplayWidget;
class EmulatorThread;
class AudioIO;
class InputManager;
class SettingsDialog;
class QCloseEvent;
class QKeyEvent;
class QMouseEvent;
class QCommandLineParser;

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QCommandLineParser *parser, QWidget *parent = nullptr);
	~MainWindow();

      protected:
	void closeEvent(QCloseEvent *ev) override;
	void keyPressEvent(QKeyEvent *ev) override;
	void keyReleaseEvent(QKeyEvent *ev) override;
	void mousePressEvent(QMouseEvent *ev) override;
	void mouseMoveEvent(QMouseEvent *ev) override;
	void mouseReleaseEvent(QMouseEvent *ev) override;

      private:
	void init_actions();
	void init_menus();
	void init_default_values();
	void set_display_size(int w, int h);
	void auto_resize_display();
	std::optional<std::pair<int, int>> get_nds_coords(QMouseEvent *ev);
	void process_event(const Event::Error& ev);
	void process_event(const Event::Shutdown& ev);
	void process_event(const Event::Restore& ev);
	void process_event(const Event::File& ev);
	void process_event(const Event::SaveType& ev);
	void process_event(const Event::EndFrame& ev);
	bool confirm_shutdown();

      private slots:
	void process_main_event(const Event::MainEvent& ev);
	void open_rom();
	void open_system_files();
	void load_save_file();
	void insert_cart();
	void eject_cart();
	void unload_save_file();
	void set_savetype(int type);
	void set_scale(int scale);
	void set_orientation(int orientation);
	void reset_to_rom();
	void reset_to_firmware();
	void reset_emulation(bool direct);
	void shutdown_emulation();
	void restore_instance();
	void toggle_pause(bool checked);
	void toggle_fastforward(bool checked);
	void toggle_linear_filtering(bool checked);
	void toggle_lock_aspect_ratio(bool checked);
	void toggle_interpolate_audio(bool checked);
	void update_title();
	void open_settings();
	void config_var_set(int key, const QVariant& v);

      private:
	ConfigManager *cfg{};
	DisplayWidget *display{};
	AudioIO *audio_out{};
	EmulatorThread *emu_thread{};
	InputManager *input{};
	SettingsDialog *settings_dialog{};
	std::vector<QAction *> actions;
	std::vector<QActionGroup *> action_groups;
	std::vector<QMenu *> menus;

	double avg_frametime{ 0.16 };
	double avg_usage[4]{};
	SharedBuffers bufs;
	bool shutdown{};
	bool restore_possible{};
	int loaded_files{};
};

#endif
