#ifndef TWICE_QT_SETTINGS_DIALOG_H
#define TWICE_QT_SETTINGS_DIALOG_H

#include <QDialog>

class QAbstractButton;
class QListWidget;
class QStackedWidget;
class QListWidgetItem;

class SettingsDialog : public QDialog {
	Q_OBJECT

      public:
	SettingsDialog(QWidget *parent);
	~SettingsDialog();
	void add_page(const QString& name, QWidget *page);

      private:
	void page_changed(QListWidgetItem *curr, QListWidgetItem *prev);
	void apply_settings();

      private slots:
	void button_accepted();
	void button_rejected();
	void apply_button_clicked();

      private:
	QListWidget *page_list{};
	QStackedWidget *pages{};
};

#endif
