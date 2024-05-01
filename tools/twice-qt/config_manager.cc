#include "config_manager.h"
#include "shaders/shaders.h"

#include "libtwice/exception.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

#include <map>

using namespace ConfigVariable;

struct config_data {
	QString key;
	QVariant default_value;
};

static const QVariant default_value;

static std::map<int, QVariant> default_cfg = {
	{ ORIENTATION, 0 },
	{ LOCK_ASPECT_RATIO, true },
	{ LINEAR_FILTERING, false },
	{ SHADER, Shader::NONE },
	{ USE_16_BIT_AUDIO, true },
	{ INTERPOLATE_AUDIO, false },
	{ DATA_DIR, "" },
	{ ARM9_BIOS_PATH, "" },
	{ ARM7_BIOS_PATH, "" },
	{ FIRMWARE_PATH, "" },
	{ IMAGE_PATH, "" },
};

static const std::map<int, QString> key_to_str = {
	{ ORIENTATION, "orientation" },
	{ LOCK_ASPECT_RATIO, "lock_aspect_ratio" },
	{ LINEAR_FILTERING, "linear_filtering" },
	{ SHADER, "shader" },
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
	{ GC_A, "gc_a" },
	{ GC_B, "gc_b" },
	{ GC_SELECT, "gc_select" },
	{ GC_START, "gc_start" },
	{ GC_RIGHT, "gc_right" },
	{ GC_LEFT, "gc_left" },
	{ GC_UP, "gc_up" },
	{ GC_DOWN, "gc_down" },
	{ GC_R, "gc_r" },
	{ GC_L, "gc_l" },
	{ GC_X, "gc_x" },
	{ GC_Y, "gc_y" },
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
	set_defaults();

	QSettings settings;

	for (const auto& [key, s] : key_to_str) {
		if (settings.contains(s)) {
			auto v = settings.value(s);
			if (is_valid(key, v)) {
				cfg[key] = v;
			}
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
			key == LINEAR_FILTERING || key == SHADER) {
		emit display_key_set(key, v);
	}

	if (KEY_A <= key && key <= GC_Y) {
		emit nds_bind_set(key, v);
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

bool
ConfigManager::is_valid(int key, const QVariant& v)
{
	switch (key) {
	case ORIENTATION:
	{
		int orientation = v.toInt();
		return 0 <= orientation && orientation <= 3;
		break;
	}
	case SHADER:
	{
		int shader = v.toInt();
		return 0 <= shader && shader < Shader::NUM_SHADERS;
		break;
	}
	}

	return true;
}

void
ConfigManager::set_defaults()
{
	auto data_dirs = QStandardPaths::standardLocations(
			QStandardPaths::AppDataLocation);
	if (!data_dirs.empty()) {
		default_cfg[DATA_DIR] = data_dirs[0];
	}

	default_cfg[ARM9_BIOS_PATH] = QStandardPaths::locate(
			QStandardPaths::AppDataLocation, "bios9.bin");
	default_cfg[ARM7_BIOS_PATH] = QStandardPaths::locate(
			QStandardPaths::AppDataLocation, "bios7.bin");
	default_cfg[FIRMWARE_PATH] = QStandardPaths::locate(
			QStandardPaths::AppDataLocation, "firmware.bin");
	default_cfg[IMAGE_PATH] = QStandardPaths::locate(
			QStandardPaths::AppDataLocation, "card.img");
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
