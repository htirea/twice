#include "emulator_thread.h"

#include "libtwice/exception.h"

#include <QSettings>
#include <iostream>

using namespace twice;

EmulatorThread::EmulatorThread(QObject *parent) : QThread(parent)
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

	while (running) {
		process_events();
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
EmulatorThread::process_event(const StopThreadEvent&)
{
	running = false;
}
