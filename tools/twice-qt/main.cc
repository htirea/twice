#include "mainwindow.h"

#include <QApplication>
#include <iostream>

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

	twice::MainWindow window(&settings, &parser);
	window.start_emulator_thread();

	window.show();
	return app.exec();
}
