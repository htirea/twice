#include "mainwindow.h"

#include <iostream>

#include <QApplication>
#include <QStandardPaths>

static void setup_default_settings(QSettings *settings);

int
main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("twice");
	QCoreApplication::setApplicationName("twice-qt");

	QCommandLineParser parser;
	parser.setApplicationDescription("Twice emulator");
	parser.addHelpOption();
	parser.addPositionalArgument("FILE",
			QApplication::translate("main", "NDS ROM file"));
	parser.process(app);

	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings settings;
	setup_default_settings(&settings);

	twice::MainWindow window(&settings, &parser);
	window.start_emulator_thread();

	window.show();
	return app.exec();
}

static void
setup_default_settings(QSettings *settings)
{
	if (!settings->contains("data_dir")) {
		auto paths = QStandardPaths::standardLocations(
				QStandardPaths::AppDataLocation);
		if (!paths.isEmpty()) {
			settings->setValue("data_dir", paths[0]);
		}
	}
}
