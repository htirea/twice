#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>

class DisplayWidget;

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

      private:
	DisplayWidget *display{};
};

#endif
