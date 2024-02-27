#include "config_manager.h"

#include "libtwice/exception.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QVariant>

#include <map>
#include <set>

using namespace ConfigVariable;

struct config_data {
	QString key;
	QVariant default_value;
};

static const QVariant default_value;

static const std::map<int, QVariant> default_cfg = {
	{ ORIENTATION, 0 },
	{ LOCK_ASPECT_RATIO, true },
	{ LINEAR_FILTERING, false },
	{ USE_16_BIT_AUDIO, true },
	{ INTERPOLATE_AUDIO, false },
	{ DATA_DIR, "" },
	{ ARM9_BIOS_PATH, "" },
	{ ARM7_BIOS_PATH, "" },
	{ FIRMWARE_PATH, "" },
	{ IMAGE_PATH, "" },
	{ KEY_A, Qt::Key_X },
	{ KEY_B, Qt::Key_Z },
	{ KEY_SELECT, Qt::Key_2 },
	{ KEY_START, Qt::Key_1 },
	{ KEY_RIGHT, Qt::Key_Right },
	{ KEY_LEFT, Qt::Key_Left },
	{ KEY_UP, Qt::Key_Up },
	{ KEY_DOWN, Qt::Key_Down },
	{ KEY_R, Qt::Key_W },
	{ KEY_L, Qt::Key_Q },
	{ KEY_X, Qt::Key_S },
	{ KEY_Y, Qt::Key_A },
};

static const std::map<int, QString> key_to_str = {
	{ ORIENTATION, "orientation" },
	{ LOCK_ASPECT_RATIO, "lock_aspect_ratio" },
	{ LINEAR_FILTERING, "linear_filtering" },
	{ USE_16_BIT_AUDIO, "use_16_bit_audio" },
	{ INTERPOLATE_AUDIO, "interpolate_audio" },
	{ DATA_DIR, "data_dir" },
	{ ARM9_BIOS_PATH, "arm9_bios" },
	{ ARM7_BIOS_PATH, "arm7_bios" },
	{ FIRMWARE_PATH, "firmware" },
	{ IMAGE_PATH, "image" },
	{ KEY_A, "key_a" },
	{ KEY_B, "key_b" },
	{ KEY_SELECT, "key_select" },
	{ KEY_START, "key_start" },
	{ KEY_RIGHT, "key_right" },
	{ KEY_LEFT, "key_left" },
	{ KEY_UP, "key_up" },
	{ KEY_DOWN, "key_down" },
	{ KEY_R, "key_r" },
	{ KEY_L, "key_l" },
	{ KEY_X, "key_x" },
	{ KEY_Y, "key_y" },
};

static const QList<QCommandLineOption> parser_options = {
	{ { "b", "boot" },
			"Set the boot mode. Defaults to 'auto'.\n"
			"(mode = 'auto' | 'never' | 'direct' | 'firmware')",
			"mode", "auto" },
};

static const std::map<int, std::vector<QString>> parser_valid_values = {
	{ CliArg::BOOT_MODE, { "auto", "never", "direct", "firmware" } },
};

void
add_command_line_arguments(QCommandLineParser& parser, QApplication& app)
{
	parser.addHelpOption();
	parser.addOptions(parser_options);
	parser.addPositionalArgument("file", "The NDS file to open.");
	parser.process(app);
}

ConfigManager::ConfigManager(QCommandLineParser *parser, QObject *parent)
	: QObject(parent)
{
	QSettings settings;

	for (const auto& [key, s] : key_to_str) {
		if (settings.contains(s)) {
			cfg[key] = settings.value(s);
		}
	}

	auto pos_args = parser->positionalArguments();
	if (pos_args.size() > 0) {
		args[CliArg::NDS_FILE] = pos_args[0];
	}

	check_and_add_parsed_arg(parser, CliArg::BOOT_MODE, "boot");
}

ConfigManager::~ConfigManager()
{
	QSettings settings;

	for (const auto& [key, v] : cfg) {
		auto s = key_to_str.at(key);
		settings.setValue(s, v);
	}
}

void
ConfigManager::emit_key_set_signal(int key, const QVariant& v)
{
	emit key_set(key, v);

	if (key == ORIENTATION || key == LOCK_ASPECT_RATIO ||
			key == LINEAR_FILTERING) {
		emit display_key_set(key, v);
	}

	if (KEY_A <= key && key <= KEY_Y) {
		emit nds_key_set(key, v);
	}
}

void
ConfigManager::emit_all_signals()
{
	for (int key = 0; key < NUM_CONFIG_VARS; key++) {
		emit_key_set_signal(key, get(key));
	}
}

const QVariant&
ConfigManager::get(int key)
{
	auto it = cfg.find(key);
	if (it != cfg.end()) {
		return it->second;
	}

	auto def_it = default_cfg.find(key);
	if (def_it != default_cfg.end()) {
		return def_it->second;
	}

	return default_value;
}

void
ConfigManager::set(int key, const QVariant& v)
{
	cfg[key] = v;
	emit_key_set_signal(key, v);
}

const QVariant&
ConfigManager::get_arg(int key)
{
	auto it = args.find(key);
	if (it != args.end()) {
		return it->second;
	}

	return default_value;
}

void
ConfigManager::check_and_add_parsed_arg(
		QCommandLineParser *parser, int key, const QString& name)
{
	auto& valid_values = parser_valid_values.at(key);
	const auto& value = parser->value(name);

	auto it = std::find(valid_values.begin(), valid_values.end(), value);
	if (it == valid_values.end()) {
		qCritical().nospace() << "Invalid value for option " << name
				      << ": " << value;
		throw twice::twice_error("Error while processing arguments.");
	}

	args[key] = (int)(it - valid_values.begin());
}
