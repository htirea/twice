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
		.use_16_bit_audio = cfg->get(USE_16_BIT_AUDIO).toBool(),
	};

	nds = std::make_unique<nds_machine>(nds_cfg);
	emit send_main_event(FileEvent{ nds->get_loaded_files() });
}

EmulatorThread::~EmulatorThread(){};

void
EmulatorThread::push_event(const Event& ev)
{
	event_q.push(ev);
}

void
EmulatorThread::run()
{
	running = true;
	throttle = true;
	paused = false;
	shutdown = true;

	frame_timer tmr(std::chrono::nanoseconds(
			(u64)(1000000000 / NDS_FRAME_RATE)));
	stopwatch frametime_tmr;
	s16 mic_buffer[548]{};
	nds_exec exec_in;
	exec_in.audio_buf = mic_buffer;
	exec_in.audio_buf_len = 548;
	nds_exec exec_out;

	while (running) {
		tmr.start_frame();
		frametime_tmr.start();
		process_events();

		bool run_frame = !shutdown && !paused;

		bufs->mb.read_fill_zero_locked((char *)mic_buffer, 548 * 2);

		if (run_frame) {
			nds->run_until_vblank(&exec_in, &exec_out);
			shutdown = exec_out.sig_flags & nds_signal::SHUTDOWN;
			emit send_main_event(ShutdownEvent{ shutdown });
		}

		size_t audio_buf_size = 0;
		if (run_frame && throttle) {
			audio_buf_size = exec_out.audio_buf_len << 2;
			std::memcpy(bufs->ab.get_write_buffer().data(),
					exec_out.audio_buf, audio_buf_size);
			bufs->ab.swap_write_buffer();
		} else {
			audio_buf_size = 0;
			bufs->ab.get_write_buffer().fill(0);
			bufs->ab.swap_write_buffer();
		}

		if (run_frame) {
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

		emit send_main_event(PushAudioEvent{ audio_buf_size });
		emit send_main_event(RenderEvent());
		emit send_main_event(EndFrameEvent{
				to_seconds<double>(frametime_tmr.measure()) });
	}
}

void
EmulatorThread::process_events()
{
	Event ev;
	while (event_q.try_pop(ev)) {
		std::visit([this](const auto& ev) { process_event(ev); }, ev);
	}
}

void
EmulatorThread::process_event(const EmptyEvent&)
{
}

void
EmulatorThread::process_event(const LoadFileEvent& ev)
{
	try {
		nds->load_file(ev.pathname.toStdU16String(), ev.type);
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ tr(err.what()) });
	}

	int loaded_files = nds->get_loaded_files();
	emit send_main_event(FileEvent{ loaded_files });

	if (ev.type == nds_file::CART_ROM) {
		emit send_main_event(SaveTypeEvent{ nds->get_savetype() });
	}
}

void
EmulatorThread::process_event(const UnloadFileEvent& ev)
{
	try {
		nds->unload_file(ev.type);
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ tr(err.what()) });
	}

	int loaded_files = nds->get_loaded_files();
	emit send_main_event(FileEvent{ loaded_files });

	if (ev.type == nds_file::CART_ROM) {
		emit send_main_event(SaveTypeEvent{ nds->get_savetype() });
	}
}

void
EmulatorThread::process_event(const SaveTypeEvent& ev)
{
	try {
		nds->set_savetype(ev.type);
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ tr(err.what()) });
	}

	emit send_main_event(SaveTypeEvent{ nds->get_savetype() });
}

void
EmulatorThread::process_event(const ResetEvent& ev)
{
	try {
		nds->reboot(ev.direct);
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ tr(err.what()) });
	}

	shutdown = nds->is_shutdown();
	emit send_main_event(ShutdownEvent{ shutdown });
	emit send_main_event(FileEvent{ nds->get_loaded_files() });
	emit send_main_event(SaveTypeEvent{ nds->get_savetype() });
}

void
EmulatorThread::process_event(const ShutdownEvent&)
{
	try {
		nds->shutdown();
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ tr(err.what()) });
	}

	shutdown = nds->is_shutdown();
	emit send_main_event(ShutdownEvent{ shutdown });
}

void
EmulatorThread::process_event(const PauseEvent& ev)
{
	paused = ev.paused;
}

void
EmulatorThread::process_event(const FastForwardEvent& ev)
{
	throttle = !ev.fastforward;
}

void
EmulatorThread::process_event(const ButtonEvent& ev)
{
	try {
		nds->update_button_state(ev.button, ev.down);
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ tr(err.what()) });
	}
}

void
EmulatorThread::process_event(const TouchEvent& ev)
{
	try {
		nds->update_touchscreen_state(
				ev.x, ev.y, ev.down, ev.quicktap, false);
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ tr(err.what()) });
	}
}

void
EmulatorThread::process_event(const StopThreadEvent&)
{
	running = false;
}
