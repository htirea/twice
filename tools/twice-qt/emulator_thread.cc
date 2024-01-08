#include "emulator_thread.h"

namespace twice {

EmulatorThread::EmulatorThread(QSettings *settings, DisplayWidget *display,
		triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer,
		triple_buffer<std::array<s16, 2048>> *abuffer,
		threaded_queue<Event> *event_q)
	: settings(settings),
	  display(display),
	  tbuffer(tbuffer),
	  abuffer(abuffer),
	  event_q(event_q)
{
	nds_config config;
	/* TODO: fix file path handling */
	config.data_dir = settings->value("data_dir").toString().toStdString();

	nds = std::make_unique<nds_machine>(config);
}

void
EmulatorThread::run()
{
	throttle = true;

	frame_timer tmr(std::chrono::nanoseconds(
			(u64)(1000000000 / NDS_FRAME_RATE)));

	while (!quit) {
		tmr.start_interval();

		handle_events();

		if (state == STOPPED) {
			tbuffer->get_write_buffer().fill(0);
			tbuffer->swap_write_buffer();
		} else if (state == PAUSED) {
			;
		} else if (state == RUNNING) {
			nds->run_frame();
			std::memcpy(tbuffer->get_write_buffer().data(),
					nds->get_framebuffer(),
					NDS_FB_SZ_BYTES);
			tbuffer->swap_write_buffer();
		}

		emit render_frame();

		if (state == RUNNING && throttle) {
			queue_audio(nds->get_audio_buffer(),
					nds->get_audio_buffer_size(),
					tmr.get_last_period());
		}

		tmr.end_interval();

		if (state == STOPPED || state == PAUSED ||
				(state == RUNNING && throttle)) {
			tmr.throttle();
		}

		emit end_frame();
	}
}

void
EmulatorThread::wait()
{
	event_q->push(QuitEvent{});
	QThread::wait();
}

void
EmulatorThread::queue_audio(
		s16 *buffer, size_t len, frame_timer::duration last_period)
{
	std::memcpy(abuffer->get_write_buffer().data(), buffer, len);
	abuffer->swap_write_buffer();

	emit push_audio(len);
}

void
EmulatorThread::handle_events()
{
	Event e;
	while (event_q->try_pop(e)) {
		std::visit([this](const auto& e) { return handle_event(e); },
				e);
	}
}

void
EmulatorThread::handle_event(const DummyEvent&)
{
}

void
EmulatorThread::handle_event(const QuitEvent&)
{
	if (state == STOPPED) {
		quit = true;
	} else {
		event_q->push(StopEvent{});
		event_q->push(QuitEvent{});
	}
}

void
EmulatorThread::handle_event(const LoadFileEvent&)
{
}

void
EmulatorThread::handle_event(const LoadROMEvent& e)
{
	nds->load_cartridge(e.pathname);
}

void
EmulatorThread::handle_event(const SetSavetypeEvent& e)
{
	nds->set_savetype(e.type);
}

void
EmulatorThread::handle_event(const BootEvent& e)
{
	nds->boot(e.direct);
	state = RUNNING;
}

void
EmulatorThread::handle_event(const PauseEvent&)
{
	if (state == RUNNING) {
		state = PAUSED;
	}
}

void
EmulatorThread::handle_event(const ResumeEvent&)
{
	if (state == PAUSED) {
		state = RUNNING;
	}
}

void
EmulatorThread::handle_event(const StopEvent&)
{
	/* TODO: handle stop */
	state = STOPPED;
}

void
EmulatorThread::handle_event(const ResetEvent&)
{
}

void
EmulatorThread::handle_event(const RotateEvent&)
{
}

void
EmulatorThread::handle_event(const ButtonEvent& e)
{
	nds->button_event(e.which, e.down);
}

void
EmulatorThread::handle_event(const TouchEvent& e)
{
	nds->update_touchscreen_state(e.x, e.y, e.down);
}

void
EmulatorThread::handle_event(const UpdateRTCEvent&)
{
}

} // namespace twice
