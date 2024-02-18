#ifndef TWICE_QT_SETTINGS_DIALOG_H
#define TWICE_QT_SETTINGS_DIALOG_H

#include <QDialog>

class QAbstractButton;

class SettingsDialog : public QDialog {
	Q_OBJECT

      public:
	SettingsDialog(QWidget *parent);
	~SettingsDialog();

      private:
	void apply_settings();

      private slots:
	void button_accepted();
	void button_rejected();
	void apply_button_clicked();
};

#endif
