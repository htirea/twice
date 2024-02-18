#include "settings_dialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::Dialog)
{
	auto layout = new QGridLayout(this);
	auto button_box = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	{
		auto apply_button =
				button_box->addButton(QDialogButtonBox::Apply);
		connect(apply_button, &QPushButton::clicked, this,
				&SettingsDialog::apply_button_clicked);
	}
	connect(button_box, &QDialogButtonBox::accepted, this,
			&SettingsDialog::button_accepted);
	connect(button_box, &QDialogButtonBox::rejected, this,
			&SettingsDialog::button_rejected);

	layout->addWidget(button_box, 1, 0, 1, 2);
}

SettingsDialog::~SettingsDialog() {}

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
