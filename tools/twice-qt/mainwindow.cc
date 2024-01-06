#include "mainwindow.h"

namespace twice {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	display = new Display();
	setCentralWidget(display);
}

MainWindow::~MainWindow() {}

} // namespace twice
