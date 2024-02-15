#include "emulator_thread.h"

#include "libtwice/util/stopwatch.h"

namespace twice {

EmulatorThread::EmulatorThread(QSettings *settings, DisplayWidget *display,
		triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer,
		triple_buffer<std::array<s16, 2048>> *abuffer,
		AudioBufferDevice *mic_io_device,
		threaded_queue<Event> *event_q)
	: settings(settings),
	  display(display),
	  tbuffer(tbuffer),
	  abuffer(abuffer),
	  mic_io_device(mic_io_device),
	  event_q(event_q)
{
	nds_config config;
	/* TODO: fix file path handling */
	config.data_dir = settings->value("data_dir").toString().toStdString();
	config.arm9_bios_path = settings->value("arm9_bios_path")
	                                        .toString()
	                                        .toStdString();
	config.arm7_bios_path = settings->value("arm7_bios_path")
	                                        .toString()
	                                        .toStdString();
	config.firmware_path = settings->value("firmware_path")
	                                       .toString()
	                                       .toStdString();

	nds = std::make_unique<nds_machine>(config);
	nds->set_use_16_bit_audio(true);
}

void
EmulatorThread::run()
{
	throttle = true;

	frame_timer tmr(std::chrono::nanoseconds(
			(u64)(1000000000 / NDS_FRAME_RATE)));
	stopwatch video_tmr;

	s16 mic_buffer[548]{};
	nds_exec exec_in;
	exec_in.audio_buf = mic_buffer;
	exec_in.audio_buf_len = 548;

	while (!quit) {
		tmr.start_frame();
		video_tmr.start();

		handle_events();

		mic_io_device->read((char *)exec_in.audio_buf, 548 * 2);

		if (!nds->is_shutdown() && !paused) {
			nds->run_until_vblank(&exec_in, nullptr);
		}

		if (nds->is_shutdown()) {
			tbuffer->get_write_buffer().fill(0);
			tbuffer->swap_write_buffer();
		} else if (paused) {
			;
		} else {
			std::memcpy(tbuffer->get_write_buffer().data(),
					nds->get_framebuffer(),
					NDS_FB_SZ_BYTES);
			tbuffer->swap_write_buffer();
			queue_audio(nds->get_audio_buffer(),
					nds->get_audio_buffer_size());
		}

		emit render_frame();

		if (nds->is_shutdown() || paused || throttle) {
			tmr.wait_until_target();
		}

		emit end_frame(to_seconds<double>(video_tmr.measure()));
	}
}

void
EmulatorThread::wait()
{
	event_q->push(QuitEvent{});
	QThread::wait();
}

void
EmulatorThread::queue_audio(s16 *buffer, size_t len)
{
	if (!throttle)
		return;

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
	if (nds->is_shutdown()) {
		quit = true;
	} else {
		event_q->push(StopEvent{});
		event_q->push(QuitEvent{});
	}
}

void
EmulatorThread::handle_event(const LoadFileEvent& e)
{
	try {
		auto type = nds_file::UNKNOWN;
		switch (e.type) {
		case LoadFileEvent::ARM9_BIOS:
			type = nds_file::ARM9_BIOS;
			break;
		case LoadFileEvent::ARM7_BIOS:
			type = nds_file::ARM7_BIOS;
			break;
		case LoadFileEvent::FIRMWARE:
			type = nds_file::FIRMWARE;
			break;
		}
		nds->load_file(e.pathname, type);
	} catch (const twice_exception& err) {
		emit show_error_msg(tr(err.what()));
	}
}

void
EmulatorThread::handle_event(const LoadROMEvent& e)
{
	try {
		nds->load_cartridge(e.pathname);
	} catch (const twice_exception& err) {
		emit show_error_msg(tr(err.what()));
	}
}

void
EmulatorThread::handle_event(const SetSavetypeEvent& e)
{
	nds->set_savetype(e.type);
}

void
EmulatorThread::handle_event(const PauseEvent&)
{
	paused = true;
}

void
EmulatorThread::handle_event(const ResumeEvent&)
{
	paused = false;
}

void
EmulatorThread::handle_event(const StopEvent&)
{
	nds->shutdown();
}

void
EmulatorThread::handle_event(const SetFastForwardEvent& e)
{
	throttle = !e.fast_forward;
}

void
EmulatorThread::handle_event(const ResetEvent& e)
{
	try {
		nds->reboot(e.direct);
	} catch (const twice_exception& err) {
		emit show_error_msg(tr(err.what()));
	}
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
	nds->update_touchscreen_state(e.x, e.y, e.down, e.quicktap, e.move);
}

void
EmulatorThread::handle_event(const UpdateRTCEvent&)
{
}

} // namespace twice
