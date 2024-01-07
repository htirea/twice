#include "mainwindow.h"

#include <QApplication>
#include <iostream>

int
main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("twice");
	QCoreApplication::setApplicationName("twice-qt");
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings settings;

	twice::MainWindow window(&settings);
	window.start_emulator_thread();
	window.show();
	return app.exec();
}
