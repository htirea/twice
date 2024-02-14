#ifndef TWICE_QT_MAINWINDOW_H
#define TWICE_QT_MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow {
	Q_OBJECT

      public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();
};

#endif
