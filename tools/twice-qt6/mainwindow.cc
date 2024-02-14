#include "mainwindow.h"

#include "actions.h"
#include "display_widget.h"

#include "libtwice/nds/defs.h"

#include <QMenuBar>

using namespace twice;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	display = new DisplayWidget(this);
	setCentralWidget(display);

	init_menus();
	init_default_values();
}

MainWindow::~MainWindow() {}

void
MainWindow::init_menus()
{
	auto video_menu = menuBar()->addMenu(tr("Video"));
	{
		auto action = create_action(ACTION_AUTO_RESIZE, this,
				&MainWindow::auto_resize_display);
		video_menu->addAction(action);
	}
	{
		auto menu = video_menu->addMenu(tr("Scale"));
		auto group = new QActionGroup(this);
		for (int i = 0; i < 4; i++) {
			int id = ACTION_SCALE_1X + i;
			menu->addAction(create_action(id, group));
		}

		auto set_scale = [=, this](QAction *action) {
			set_display_size(action->data().toInt());
		};
		connect(group, &QActionGroup::triggered, this, set_scale);
	}
	{
		auto menu = video_menu->addMenu(tr("Orientation"));
		menu->addAction(get_action(ACTION_ORIENTATION_0));
		menu->addAction(get_action(ACTION_ORIENTATION_1));
		menu->addAction(get_action(ACTION_ORIENTATION_3));
		menu->addAction(get_action(ACTION_ORIENTATION_2));
	}
	video_menu->addAction(get_action(ACTION_LINEAR_FILTERING));
	video_menu->addAction(get_action(ACTION_LOCK_ASPECT_RATIO));
}

void
MainWindow::init_default_values()
{
	get_action(ACTION_ORIENTATION_0)->trigger();
	get_action(ACTION_LOCK_ASPECT_RATIO)->trigger();
	set_display_size(NDS_FB_W * 2, NDS_FB_H * 2);
}

void
MainWindow::set_display_size(int w, int h)
{
	display->setFixedSize(w, h);
	setFixedSize(sizeHint());
	display->setMinimumSize(0, 0);
	display->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	setMinimumSize(0, 0);
	setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void
MainWindow::auto_resize_display()
{
	auto size = display->size();
	double w = size.width();
	double h = size.height();
	double ratio = (double)w / h;
	double t = get_orientation() & 1 ? NDS_FB_ASPECT_RATIO_RECIP
	                                 : NDS_FB_ASPECT_RATIO;

	if (ratio < t) {
		h = w / t;
	} else if (ratio > t) {
		w = h * t;
	}

	set_display_size(w, h);
}

void
MainWindow::set_display_size(int scale)
{
	int w = NDS_FB_W * scale;
	int h = NDS_FB_H * scale;

	if (get_orientation() & 1) {
		std::swap(w, h);
	}

	set_display_size(w, h);
}
