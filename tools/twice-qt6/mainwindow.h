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
	void process_event(const CartChangeEvent& ev);
	void process_event(const EndFrameEvent& ev);

      private slots:
	void process_main_event(const MainEvent& ev);
	void open_rom();
	void open_system_files();
	void reset_emulation(bool direct);
	void shutdown_emulation();
	void toggle_pause(bool checked);
	void toggle_fastforward(bool checked);

      private:
	DisplayWidget *display{};
	EmulatorThread *emu_thread{};
	SharedBuffers bufs;
	bool shutdown{};
	bool cart_loaded{};
};

#endif
