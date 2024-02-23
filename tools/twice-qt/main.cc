#include "mainwindow.h"

#include "config_manager.h"

#include "libtwice/exception.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QSurfaceFormat>

int
main(int argc, char *argv[])
try {
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("twice");
	QCoreApplication::setApplicationName("twice-qt");

	QCommandLineParser parser;
	add_command_line_arguments(parser, app);

	QSettings::setDefaultFormat(QSettings::IniFormat);

	QPalette palette;
	palette.setColor(QPalette::Disabled, QPalette::WindowText,
			QColorConstants::Svg::dimgray);
	palette.setColor(QPalette::Disabled, QPalette::Text,
			QColorConstants::Svg::dimgray);
	app.setPalette(palette);

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	auto w = std::make_unique<MainWindow>(&parser);
	w->show();
	return app.exec();
} catch (const twice::twice_exception& err) {
	qCritical() << err.what();
	return 1;
}
