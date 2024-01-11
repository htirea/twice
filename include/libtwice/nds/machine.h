#ifndef LIBTWICE_NDS_MACHINE_H
#define LIBTWICE_NDS_MACHINE_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

#include "libtwice/nds/defs.h"
#include "libtwice/nds/game_db.h"
#include "libtwice/util/filemap.h"

namespace twice {

struct nds_ctx;

/**
 * The configuration to use when creating the NDS machine.
 */
struct nds_config {
	std::filesystem::path data_dir;
	std::filesystem::path arm9_bios_path;
	std::filesystem::path arm7_bios_path;
	std::filesystem::path firmware_path;
	bool use_16_bit_audio{};
};

/**
 * The buttons of the NDS machine.
 */
enum class nds_button {
	A,
	B,
	SELECT,
	START,
	RIGHT,
	LEFT,
	UP,
	DOWN,
	R,
	L,
	X,
	Y,
	NONE,
};

/*
 * The system files of the NDS machine.
 */
enum class nds_system_file {
	ARM9_BIOS,
	ARM7_BIOS,
	FIRMWARE,
};

/**
 * Represents an NDS machine.
 */
struct nds_machine {
	/**
	 * Create the machine.
	 *
	 * The following files will be searched for in the directory
	 * specified by `data_dir` in the configuration:
	 * `bios7.bin`: the arm7 bios
	 * `bios9.bin`: the arm9 bios
	 * `firmware.bin`: the NDS firmware
	 *
	 * \param config the configuration to use
	 */
	nds_machine(const nds_config& config);
	~nds_machine();

	/**
	 * Load a system file into the machine.
	 *
	 * \param pathname the file to load
	 * \param type the type of system file
	 */
	void load_system_file(const std::filesystem::path& pathname,
			nds_system_file type);

	/**
	 * Load a cartridge into the machine.
	 *
	 * \param pathname the file to load
	 */
	void load_cartridge(const std::filesystem::path& pathname);

	/**
	 * Eject the cartridge from the machine.
	 *
	 * If there is no cartridge this function does nothing.
	 */
	void eject_cartridge();

	/**
	 * Set the save type of the cartridge.
	 *
	 * The save type must be set before booting the machine.
	 *
	 * \param savetype the save type for the cartridge
	 */
	void set_savetype(nds_savetype savetype);

	/**
	 * Load a save file.
	 *
	 * The save file must exist. If the save type is not set, the save type
	 * will be automatically guessed from the file size.
	 *
	 * \param pathname the file to load
	 */
	void load_savefile(const std::filesystem::path& pathname);

	/**
	 * Load or create a save file.
	 *
	 * This function first tries to load the save file. If the loading
	 * fails, it will create the save file using the given `savetype`.
	 *
	 * The `savetype` must be set if the save file is created.
	 *
	 * \param pathname the file to load
	 * \param savetype the savetype to use if creating
	 */
	void load_or_create_savefile(const std::filesystem::path& pathname,
			nds_savetype try_savetype);

	/**
	 * Boot up the machine.
	 *
	 * If the machine is already running, this function will do nothing.
	 *
	 * If `direct_boot` is true, then a cartridge must already be loaded.
	 *
	 * \param direct_boot true to boot the cartridge directly,
	 *                    false to boot the firmware
	 */
	void boot(bool direct_boot);

	/**
	 * Shut down the machine.
	 *
	 * If the machine is already shut down, this function will do nothing.
	 */
	void shutdown();

	/**
	 * Reboot the machine.
	 *
	 * This function behaves as if `shutdown` and `boot` were called.
	 *
	 * \param direct_boot true to boot the cartridge directly,
	 *                    false to boot the firmware
	 */
	void reboot(bool direct_boot);

	/**
	 * Emulate one frame.
	 */
	void run_frame();

	/**
	 * Check whether the machine is shut down.
	 *
	 * \returns true if the machine is shut down
	 *          false otherwise
	 */
	bool is_shutdown();

	/**
	 * Get the rendered framebuffer.
	 *
	 * The framebuffer is stored as an array of 32 bit pixels in BGR888
	 * format, with the most significant 8 bits left unused.
	 *
	 * The framebuffer has a width of 256 pixels, and a height of 384
	 * pixels. The first half of the framebuffer contains the top screen,
	 * and the second half contains the bottom screen.
	 *
	 * \returns a pointer to the framebuffer
	 */
	u32 *get_framebuffer();

	/**
	 * Get the audio buffer for the emulated frame.
	 *
	 * The audio buffer is stored as an array of signed 16 bit samples,
	 * with 2 channels, interleaved, at a sample rate of 32768 Hz.
	 *
	 * \returns a pointer to the audio buffer
	 */
	s16 *get_audio_buffer();

	/**
	 * Get the audio buffer size for the emulated frame.
	 *
	 * The size of the audio buffer is given in bytes.
	 *
	 * \returns the buffer size in bytes
	 */
	u32 get_audio_buffer_size();

	/**
	 * Update the state of a button.
	 *
	 * \param button the button to update the state for
	 * \param down true if the button is pressed,
	 *             false if the button is released
	 */
	void button_event(nds_button button, bool down);

	/**
	 * Update the current touchscreen state.
	 *
	 * If `down` is false, then the x and y coordinates are ignored.
	 *
	 * \param x [0..255]
	 * \param y [0..191]
	 * \param down whether the pen is touching the screen
	 * \param quicktap whether the pen was tapped quickly
	 * \param moved whether the pen was moved
	 */
	void update_touchscreen_state(
			int x, int y, bool down, bool quicktap, bool moved);

	/**
	 * Update the current real world date and time used by the RTC.
	 *
	 * The date and time provided must be valid.
	 *
	 * \param year [2000..2099]
	 * \param month [1..12]
	 * \param day [1..31]
	 * \param weekday [1..7], where Monday is 1
	 * \param hour [0..23]
	 * \param minute [0..59]
	 * \param second [0..59]
	 */
	void update_real_time_clock(int year, int month, int day, int weekday,
			int hour, int minute, int second);

	/**
	 * Get the CPU usage.
	 *
	 * The CPU usage is a value in the range [0..1].
	 *
	 * \returns a pair containing the arm9 and arm7 cpu usage
	 */
	std::pair<double, double> get_cpu_usage();

	/**
	 * Set the audio mixing bit depth.
	 *
	 * \param use_16_bit_audio true to mix audio at 16 bits
	 *                         false to mix audio at 10 bits
	 */
	void set_use_16_bit_audio(bool use_16_bit_audio);

	/**
	 * Sync files to disk.
	 */
	void sync_files();

	/**
	 * Dump the collected profiler data.
	 */
	void dump_profiler_report();

      private:
	nds_config config;
	file_map arm7_bios;
	file_map arm9_bios;
	file_map firmware;
	file_map cartridge;
	file_map savefile;
	nds_savetype savetype{ SAVETYPE_UNKNOWN };
	std::unique_ptr<nds_ctx> nds;
};

} // namespace twice

#endif
