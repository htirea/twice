#include "emulatorthread.h"

#include "util/frame_timer.h"

namespace twice {

EmulatorThread::EmulatorThread(QSettings *settings, Display *display,
		triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer)
	: settings(settings), display(display), tbuffer(tbuffer)
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
	event_q.push(QuitEvent{});
	QThread::wait();
}

void
EmulatorThread::handle_events()
{
	Event e;
	while (event_q.try_pop(e)) {
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
		event_q.push(StopEvent{});
		event_q.push(QuitEvent{});
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
EmulatorThread::handle_event(const ButtonEvent&)
{
}

void
EmulatorThread::handle_event(const TouchEvent&)
{
}

void
EmulatorThread::handle_event(const UpdateRTCEvent&)
{
}

} // namespace twice
