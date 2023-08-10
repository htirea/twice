#ifndef LIBTWICE_MACHINE_H
#define LIBTWICE_MACHINE_H

#include <cstddef>
#include <memory>
#include <string>

#include "libtwice/config.h"
#include "libtwice/filemap.h"

namespace twice {

struct nds_ctx;

/**
 * The configuration to use when creating the NDS machine.
 */
struct nds_config {
	std::string data_dir;
};

/**
 * Represents an NDS machine.
 */
struct nds_machine {
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
	 * \param pathname the file to open
	 */
	void load_cartridge(const std::string& pathname);

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
	void *get_framebuffer();

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

      private:
	nds_config config;
	file_map arm7_bios;
	file_map arm9_bios;
	file_map firmware;
	file_map cartridge;
	std::unique_ptr<nds_ctx> nds;
};

} // namespace twice

#endif
