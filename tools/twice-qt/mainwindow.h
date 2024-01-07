#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <QLayout>
#include <QMainWindow>
#include <QSettings>
#include <QStandardPaths>

#include "display.h"
#include "emulatorthread.h"

namespace twice {

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QSettings *settings, QWidget *parent = nullptr);
	~MainWindow();

      private:
	Display *display{};
	QSettings *settings{};
	EmulatorThread *emu_thread{};

      public slots:
	void frame_ended();
};

} // namespace twice

#endif
