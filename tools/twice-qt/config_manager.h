#ifndef TWICE_QT_CONFIG_MANAGER_H
#define TWICE_QT_CONFIG_MANAGER_H

#include "libtwice/nds/machine.h"

#include <QObject>

/*
 * The possible config variables.
 */
namespace ConfigVariable {
enum {
	ORIENTATION,
	LOCK_ASPECT_RATIO,
	LINEAR_FILTERING,
	USE_16_BIT_AUDIO,
	INTERPOLATE_AUDIO,
	DATA_DIR,
	ARM9_BIOS_PATH,
	ARM7_BIOS_PATH,
	FIRMWARE_PATH,
	IMAGE_PATH,
	KEY_A,
	KEY_B,
	KEY_SELECT,
	KEY_START,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_UP,
	KEY_DOWN,
	KEY_R,
	KEY_L,
	KEY_X,
	KEY_Y,
	NUM_CONFIG_VARS,
};
}

/*
 * The possible command line arguments
 */
namespace CliArg {
enum {
	NDS_FILE,
	BOOT_MODE,
};

enum {
	BOOT_AUTO,
	BOOT_NEVER,
	BOOT_DIRECT,
	BOOT_FIRMWARE,
};
} // namespace CliArg

class QCommandLineParser;
class QApplication;

/*
 * The ConfigManager stores runtime configuration variables for the frontend.
 *
 * On construction, the variables are initialised from the configuration.
 * On destruction, the configuration file is written to.
 *
 * Signals are emitted whenever a variable is changed through the set
 * functions. No signals are emitted during construction.
 */
class ConfigManager : public QObject {
	Q_OBJECT

      public:
	ConfigManager(QCommandLineParser *parser, QObject *parent);
	~ConfigManager();
	void emit_key_set_signal(int key, const QVariant& v);
	void emit_all_signals();

	const QVariant& get(int key);
	void set(int key, const QVariant& v);
	const QVariant& get_arg(int key);

      private:
	void check_and_add_parsed_arg(QCommandLineParser *parser, int key,
			const QString& name);

      signals:
	void key_set(int key, const QVariant& v);
	void nds_key_set(int key, const QVariant& v);
	void display_key_set(int key, const QVariant& v);

      private:
	std::map<int, QVariant> cfg;
	std::map<int, QVariant> args;
};

void add_command_line_arguments(QCommandLineParser& parser, QApplication& app);

#endif
