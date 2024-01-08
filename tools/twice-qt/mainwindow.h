#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QDragEnterEvent>
#include <QHash>
#include <QLayout>
#include <QMainWindow>
#include <QMediaDevices>
#include <QMimeData>
#include <QSettings>
#include <QStandardPaths>

#include "display_widget.h"
#include "emulator_events.h"
#include "emulator_thread.h"
#include "util/threaded_queue.h"
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
	void initialize_commands();
	void set_default_keybinds();
	void set_nds_button_state(nds_button button, bool down);

      protected:
	void dragEnterEvent(QDragEnterEvent *) override;
	void dropEvent(QDropEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;
	void keyReleaseEvent(QKeyEvent *e) override;

      private:
	threaded_queue<Event> event_q;
	DisplayWidget *display{};
	QSettings *settings{};
	QAudioSink *audio_sink{};
	QIODevice *audio_out_buffer{};
	EmulatorThread *emu_thread{};
	triple_buffer<std::array<u32, NDS_FB_SZ>> tbuffer;
	triple_buffer<std::array<s16, 2048>> abuffer;
	QHash<QKeyCombination, int> keybinds{};
	std::unordered_map<int, std::function<void(intptr_t arg)>> cmd_map;

      public slots:
	void frame_ended();
	void render_frame();
	void push_audio(size_t len);
};

} // namespace twice

#endif
