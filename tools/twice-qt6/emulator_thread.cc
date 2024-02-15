#include "emulator_thread.h"

#include "libtwice/exception.h"
#include "libtwice/util/frame_timer.h"

#include <QSettings>
#include <iostream>

using namespace twice;

EmulatorThread::EmulatorThread(SharedBuffers *bufs, QObject *parent)
	: QThread(parent), bufs(bufs)
{
	QSettings settings;
	nds_config cfg{
		.data_dir = settings.value("data_dir")
		                            .toString()
		                            .toStdU16String(),
		.arm9_bios_path = settings.value("arm9_bios_path")
		                                  .toString()
		                                  .toStdU16String(),
		.arm7_bios_path = settings.value("arm7_bios_path")
		                                  .toString()
		                                  .toStdU16String(),
		.firmware_path = settings.value("firmware_path")
		                                 .toString()
		                                 .toStdU16String(),
		.use_16_bit_audio = true,
	};

	nds = std::make_unique<nds_machine>(cfg);
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
	nds_exec exec_in;
	nds_exec exec_out;

	while (running) {
		tmr.start_frame();
		frametime_tmr.start();
		process_events();

		if (!shutdown && !paused) {
			nds->run_until_vblank(&exec_in, &exec_out);
			shutdown = exec_out.sig_flags & nds_signal::SHUTDOWN;
			send_main_event(ShutdownEvent{ .shutdown = shutdown });
		}

		if (shutdown) {
			bufs->vb.get_write_buffer().fill(0);
			bufs->vb.swap_write_buffer();
		} else if (paused) {
			;
		} else {
			std::memcpy(bufs->vb.get_write_buffer().data(),
					exec_out.fb, NDS_FB_SZ_BYTES);
			bufs->vb.swap_write_buffer();
		}

		send_main_event(RenderEvent());

		if (shutdown || paused || throttle) {
			tmr.wait_until_target();
		}

		send_main_event(EndFrameEvent{
				.frametime = to_seconds<double>(
						frametime_tmr.measure()) });
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
EmulatorThread::process_event(const LoadROMEvent& ev)
{
	try {
		nds->load_cartridge(ev.pathname.toStdU16String());
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ .msg = tr(err.what()) });
	}

	emit send_main_event(
			CartChangeEvent{ .cart_loaded = nds->cart_loaded() });
}

void
EmulatorThread::process_event(const LoadSystemFileEvent& ev)
{
	try {
		nds->load_system_file(ev.pathname.toStdU16String(), ev.type);

		QSettings settings;
		switch (ev.type) {
		case nds_system_file::ARM9_BIOS:
			settings.setValue("arm9_bios_path", ev.pathname);
			break;
		case nds_system_file::ARM7_BIOS:
			settings.setValue("arm7_bios_path", ev.pathname);
			break;
		case nds_system_file::FIRMWARE:
			settings.setValue("firmware_path", ev.pathname);
			break;
		default:;
		}
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ .msg = tr(err.what()) });
	}
}

void
EmulatorThread::process_event(const ResetEvent& ev)
{
	try {
		nds->reboot(ev.direct);
		shutdown = false;
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ .msg = tr(err.what()) });
	}

	send_main_event(ShutdownEvent{ .shutdown = shutdown });
}

void
EmulatorThread::process_event(const ShutdownEvent&)
{
	try {
		nds->shutdown();
		shutdown = true;
	} catch (const twice_exception& err) {
		emit send_main_event(ErrorEvent{ .msg = tr(err.what()) });
	}

	send_main_event(ShutdownEvent{ .shutdown = shutdown });
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
EmulatorThread::process_event(const StopThreadEvent&)
{
	running = false;
}
