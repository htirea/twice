#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include "buffers.h"
#include "events.h"

#include <QCloseEvent>
#include <QMainWindow>

class DisplayWidget;
class EmulatorThread;

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

      protected:
	void closeEvent(QCloseEvent *ev) override;

      private:
	void init_menus();
	void init_default_values();
	void set_display_size(int w, int h);
	void set_display_size(int scale);
	void auto_resize_display();
	void process_event(const EmptyEvent& ev);
	void process_event(const ErrorEvent& ev);
	void process_event(const RenderEvent& ev);
	void process_event(const ShutdownEvent& ev);
	void process_event(const FileEvent& ev);
	void process_event(const SaveTypeEvent& ev);
	void process_event(const EndFrameEvent& ev);
	bool confirm_shutdown();

      private slots:
	void process_main_event(const MainEvent& ev);
	void open_rom();
	void open_system_files();
	void load_save_file();
	void insert_cart();
	void eject_cart();
	void unload_save_file();
	void set_savetype(int type);
	void reset_emulation(bool direct);
	void shutdown_emulation();
	void toggle_pause(bool checked);
	void toggle_fastforward(bool checked);

      private:
	DisplayWidget *display{};
	EmulatorThread *emu_thread{};
	SharedBuffers bufs;
	bool shutdown{};
	int loaded_files{};
};

#endif
