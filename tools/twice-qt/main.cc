#include "mainwindow.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QSurfaceFormat>

static void set_default_settings();

int
main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("twice");
	QCoreApplication::setApplicationName("twice-qt");

	QSettings::setDefaultFormat(QSettings::IniFormat);
	set_default_settings();

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

	MainWindow w;
	w.show();
	return app.exec();
}

static void
set_default_settings()
{
	QSettings settings;

	if (!settings.contains("data_dir")) {
		auto paths = QStandardPaths::standardLocations(
				QStandardPaths::AppDataLocation);
		if (!paths.isEmpty()) {
			settings.setValue("data_dir", paths[0]);
		}
	}
}
