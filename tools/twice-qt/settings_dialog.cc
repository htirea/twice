#include "settings_dialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::Dialog)
{
	auto layout = new QGridLayout(this);

	page_list = new QListWidget();
	pages = new QStackedWidget();

	auto button_box = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	auto apply_button = button_box->addButton(QDialogButtonBox::Apply);
	connect(apply_button, &QPushButton::clicked, this,
			&SettingsDialog::apply_button_clicked);
	connect(button_box, &QDialogButtonBox::accepted, this,
			&SettingsDialog::button_accepted);
	connect(button_box, &QDialogButtonBox::rejected, this,
			&SettingsDialog::button_rejected);

	layout->addWidget(page_list, 0, 0, 1, 1);
	layout->addWidget(pages, 0, 1, 1, 1);
	layout->addWidget(button_box, 1, 0, 1, 2);
	setLayout(layout);
}

SettingsDialog::~SettingsDialog() {}

void
SettingsDialog::add_page(const QString& name, QWidget *page)
{
	auto list_item = new QListWidgetItem(page_list);
	list_item->setText(name);
	connect(page_list, &QListWidget::currentItemChanged, this,
			&SettingsDialog::page_changed);
	pages->addWidget(page);
}

void
SettingsDialog::page_changed(QListWidgetItem *curr, QListWidgetItem *prev)
{
	if (!curr) {
		curr = prev;
	}

	pages->setCurrentIndex(page_list->row(curr));
}

void
SettingsDialog::apply_settings()
{
}

void
SettingsDialog::button_accepted()
{
	apply_settings();
	accept();
}

void
SettingsDialog::button_rejected()
{
	reject();
}

void
SettingsDialog::apply_button_clicked()
{
	apply_settings();
}
