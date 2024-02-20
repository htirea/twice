#include "mainwindow.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QSurfaceFormat>

int
main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("twice");
	QCoreApplication::setApplicationName("twice-qt");

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

	auto w = std::make_unique<MainWindow>();
	w->show();
	return app.exec();
}
