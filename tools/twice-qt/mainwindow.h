#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QDragEnterEvent>
#include <QLayout>
#include <QMainWindow>
#include <QMediaDevices>
#include <QMimeData>
#include <QSettings>
#include <QStandardPaths>

#include "display.h"
#include "emulatorthread.h"
#include "util/triple_buffer.h"

namespace twice {

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QSettings *settings, QWidget *parent = nullptr);
	~MainWindow();
	void start_emulator_thread();

      private:
	void set_display_size(int w, int h);

      protected:
	void dragEnterEvent(QDragEnterEvent *) override;
	void dropEvent(QDropEvent *e) override;

      private:
	Display *display{};
	QSettings *settings{};
	QAudioSink *audio_sink{};
	QIODevice *audio_out_buffer{};
	EmulatorThread *emu_thread{};
	triple_buffer<std::array<u32, NDS_FB_SZ>> tbuffer;
	triple_buffer<std::array<s16, 2048>> abuffer;

      public slots:
	void frame_ended();
	void audio_queued(size_t len);
};

} // namespace twice

#endif
