#include "emulator_thread.h"

#include "config_manager.h"

#include "libtwice/exception.h"
#include "libtwice/util/frame_timer.h"

#include <QVariant>

using namespace twice;

EmulatorThread::EmulatorThread(
		SharedBuffers *bufs, ConfigManager *cfg, QObject *parent)
	: QThread(parent), bufs(bufs)
{
	using namespace ConfigVariable;

	nds_config nds_cfg{
		.data_dir = cfg->get(DATA_DIR).toString().toStdU16String(),
		.arm9_bios_path = cfg->get(ARM9_BIOS_PATH)
		                                  .toString()
		                                  .toStdU16String(),
		.arm7_bios_path = cfg->get(ARM7_BIOS_PATH)
		                                  .toString()
		                                  .toStdU16String(),
		.firmware_path = cfg->get(FIRMWARE_PATH)
		                                 .toString()
		                                 .toStdU16String(),
		.image_path = cfg->get(IMAGE_PATH).toString().toStdU16String(),
		.use_16_bit_audio = cfg->get(USE_16_BIT_AUDIO).toBool(),
	};

	nds = std::make_unique<nds_machine>(nds_cfg);
	on_loaded_files_maybe_changed();
}

EmulatorThread::~EmulatorThread(){};

void
EmulatorThread::push_event(const Event::Event& ev)
{
	event_q.push(ev);
}

void
EmulatorThread::run()
{
	frame_timer tmr(std::chrono::nanoseconds(
			(u64)(1000000000 / NDS_FRAME_RATE)));
	stopwatch frametime_tmr;
	s16 mic_buffer[548]{};
	nds_exec exec_in;
	nds_exec exec_out;

	running = true;
	throttle = true;
	paused = false;
	shutdown = true;
	exec_in.audio_buf = mic_buffer;
	exec_in.audio_buf_len = 548;

	while (running) {
		tmr.start_frame();
		frametime_tmr.start();
		process_events();

		bufs->mb.read_fill_zero_locked((char *)mic_buffer, 548 * 2);

		if (!shutdown && !paused) {
			try {
				nds->run_until_vblank(&exec_in, &exec_out);
			} catch (const twice_exception& err) {
				nds->shutdown();
				emit send_main_event(Event::Error{
						tr(err.what()) });
			}
			on_shutdown_maybe_changed();
		}

		size_t audio_buf_size = 0;
		if (!shutdown && !paused && throttle) {
			audio_buf_size = exec_out.audio_buf_len << 2;
			bufs->ab.write_locked((char *)exec_out.audio_buf,
					audio_buf_size);
		} else {
			audio_buf_size = 0;
			bufs->ab.fill_locked(0);
		}

		if (!shutdown && !paused) {
			std::memcpy(bufs->vb.get_write_buffer().data(),
					exec_out.fb, NDS_FB_SZ_BYTES);
			bufs->vb.swap_write_buffer();
		} else {
			if (!paused) {
				bufs->vb.get_write_buffer().fill(0);
				bufs->vb.swap_write_buffer();
			}
		}

		if (shutdown || paused || throttle) {
			tmr.wait_until_target();
		}

		emit send_main_event(Event::EndFrame{
				to_seconds<double>(frametime_tmr.measure()),
				exec_out.cpu_usage, exec_out.dma_usage });
	}
}

void
EmulatorThread::process_events()
{
	Event::Event ev;
	while (event_q.try_pop(ev)) {
		std::visit([this](const auto& ev) { process_event(ev); }, ev);
	}
}

void
EmulatorThread::process_event(const Event::LoadFile& ev)
{
	try {
		nds->load_file(ev.pathname.toStdU16String(), ev.type);
	} catch (const twice_exception& err) {
		emit send_main_event(Event::Error{ tr(err.what()) });
	}

	on_loaded_files_maybe_changed();
	if (ev.type == nds_file::CART_ROM) {
		on_savetype_maybe_changed();
	}
}

void
EmulatorThread::process_event(const Event::UnloadFile& ev)
{
	try {
		nds->unload_file(ev.type);
	} catch (const twice_exception& err) {
		emit send_main_event(Event::Error{ tr(err.what()) });
	}

	on_loaded_files_maybe_changed();
	if (ev.type == nds_file::CART_ROM) {
		on_savetype_maybe_changed();
	}
}

void
EmulatorThread::process_event(const Event::SaveType& ev)
{
	try {
		nds->set_savetype(ev.type);
	} catch (const twice_exception& err) {
		emit send_main_event(Event::Error{ tr(err.what()) });
	}

	on_savetype_maybe_changed();
}

void
EmulatorThread::process_event(const Event::Reset& ev)
{
	try {
		nds->reboot(ev.direct);
	} catch (const twice_exception& err) {
		emit send_main_event(Event::Error{ tr(err.what()) });
	}

	on_shutdown_maybe_changed();
	on_loaded_files_maybe_changed();
	on_savetype_maybe_changed();
}

void
EmulatorThread::process_event(const Event::Shutdown&)
{
	nds->shutdown();
	on_shutdown_maybe_changed();
}

void
EmulatorThread::process_event(const Event::Restore&)
{
	try {
		nds->restore_last_instance(true);
	} catch (const twice_exception& err) {
		emit send_main_event(Event::Error{ tr(err.what()) });
	}

	on_nds_instance_maybe_changed();
}

void
EmulatorThread::process_event(const Event::Pause& ev)
{
	paused = ev.paused;
}

void
EmulatorThread::process_event(const Event::FastForward& ev)
{
	throttle = !ev.fastforward;
}

void
EmulatorThread::process_event(const Event::Button& ev)
{
	try {
		nds->update_button_state(ev.button, ev.down);
	} catch (const twice_exception& err) {
		emit send_main_event(Event::Error{ tr(err.what()) });
	}
}

void
EmulatorThread::process_event(const Event::Touch& ev)
{
	try {
		nds->update_touchscreen_state(
				ev.x, ev.y, ev.down, ev.quicktap, false);
	} catch (const twice_exception& err) {
		emit send_main_event(Event::Error{ tr(err.what()) });
	}
}

void
EmulatorThread::process_event(const Event::Audio& ev)
{
	nds->set_interpolate_audio(ev.interpolate_audio);
}

void
EmulatorThread::process_event(const Event::StopThread&)
{
	running = false;
}

void
EmulatorThread::on_shutdown_maybe_changed()
{
	shutdown = nds->is_shutdown();
	emit send_main_event(Event::Shutdown{ shutdown });

	bool restore_possible = !nds->is_last_instance_shutdown();
	emit send_main_event(Event::Restore{ restore_possible });
}

void
EmulatorThread::on_loaded_files_maybe_changed()
{
	auto loaded_files = nds->get_loaded_files();
	emit send_main_event(Event::File{ loaded_files });
}

void
EmulatorThread::on_savetype_maybe_changed()
{
	auto savetype = nds->get_savetype();
	emit send_main_event(Event::SaveType{ savetype });
}

void
EmulatorThread::on_nds_instance_maybe_changed()
{
	on_shutdown_maybe_changed();
	on_loaded_files_maybe_changed();
	on_savetype_maybe_changed();
}
