#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include "events.h"

#include <QMainWindow>

class DisplayWidget;
class EmulatorThread;

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

      private:
	void init_menus();
	void init_default_values();
	void set_display_size(int w, int h);
	void set_display_size(int scale);
	void auto_resize_display();
	void process_event(const EmptyEvent& ev);
	void process_event(const ErrorEvent& ev);

      private slots:
	void open_rom();
	void open_system_files();
	void process_main_event(const MainEvent& ev);

      private:
	DisplayWidget *display{};
	EmulatorThread *emu_thread{};
};

#endif
