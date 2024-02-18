#ifndef TWICE_QT_INPUT_CONTROL_H
#define TWICE_QT_INPUT_CONTROL_H

#include "libtwice/nds/machine.h"

#include <QObject>
#include <map>

class InputControl : public QObject {
	Q_OBJECT

      public:
	InputControl(QObject *parent);
	~InputControl();
	void set_nds_mapping(twice::nds_button button, int key);
	twice::nds_button get_nds_mapping(int key);

      private:
	std::map<int, twice::nds_button> key_to_nds;
};

#endif
