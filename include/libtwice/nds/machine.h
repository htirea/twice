#ifndef LIBTWICE_NDS_MACHINE_H
#define LIBTWICE_NDS_MACHINE_H

#include <cstddef>
#include <memory>
#include <string>

#include "libtwice/filemap.h"
#include "libtwice/nds/defs.h"
#include "libtwice/nds/game_db.h"

namespace twice {

struct nds_ctx;

/**
 * The configuration to use when creating the NDS machine.
 */
struct nds_config {
	std::string data_dir;
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

/**
 * Represents an NDS machine.
 */
struct nds_machine {
	/**
	 * Create the machine.
	 *
	 * The following files are required for initialization:
	 * `bios7.bin`: the arm7 bios
	 * `bios9.bin`: the arm9 bios
	 * `firmware.bin`: the NDS firmware
	 *
	 * These files should be placed in the directory specified by
	 * `data_dir` in the configuration.
	 *
	 * \param config the configuration to use
	 */
	nds_machine(const nds_config& config);
	~nds_machine();

	/**
	 * Load a cartridge into the machine.
	 *
	 * If `save_info` is not given, it will attempt to determine the save
	 * info automatically.
	 *
	 * \param pathname the file to open
	 * \param save_info the save info for the cartridge
	 */
	void load_cartridge(const std::string& pathname);
	void load_cartridge(
			const std::string& pathname, nds_save_info save_info);

	/**
	 * Boot up the machine.
	 *
	 * If `direct_boot` is true, then a cartridge must already be loaded.
	 *
	 * \param direct_boot true to boot the cartridge directly,
	 *                    false to boot the firmware
	 */
	void boot(bool direct_boot);

	/**
	 * Emulate one frame.
	 */
	void run_frame();

	/**
	 * Check whether the machine is shutdown.
	 *
	 * \returns true if the machine is shutdown
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
	 * \param down true if the touchscreen is pressed,
	 *             false if the touchscreen is released
	 */
	void update_touchscreen_state(int x, int y, bool down);

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
	nds_save_info save_info{ SAVETYPE_UNKNOWN, 0 };
	std::unique_ptr<nds_ctx> nds;
};

} // namespace twice

#endif
