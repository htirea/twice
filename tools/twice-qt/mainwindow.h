#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <QLayout>
#include <QMainWindow>
#include <QSettings>
#include <QStandardPaths>

#include "display.h"
#include "emulatorthread.h"
#include "triple_buffer.h"

namespace twice {

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QSettings *settings, QWidget *parent = nullptr);
	~MainWindow();
	void start_emulator_thread();

      private:
	void set_display_size(int w, int h);

      private:
	Display *display{};
	QSettings *settings{};
	EmulatorThread *emu_thread{};
	triple_buffer<std::array<u32, NDS_FB_SZ>> tbuffer;

      public slots:
	void frame_ended();
};

} // namespace twice

#endif
