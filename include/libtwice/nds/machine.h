#ifndef LIBTWICE_NDS_MACHINE_H
#define LIBTWICE_NDS_MACHINE_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

#include "libtwice/nds/defs.h"
#include "libtwice/nds/game_db.h"

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

/**
 * The file types of the NDS machine.
 */
enum class nds_file {
	UNKNOWN = 0x1,
	CART_ROM = 0x2,
	SAVE = 0x4,
	ARM9_BIOS = 0x8,
	ARM7_BIOS = 0x10,
	FIRMWARE = 0x20,
};

/**
 * The signals generated by the NDS machine.
 */
namespace nds_signal {
enum : unsigned long {
	SHUTDOWN = 1ul << 0,
	VBLANK = 1ul << 1,
};
}

/**
 * Information related to the real time clock of the NDS machine.
 *
 * Internally, the RTC uses registers which have only a limited number of bits.
 * Thus, any invalid values will first be masked to the correct number of bits,
 * and the set to a default value.
 */
struct nds_rtc_state {
	/**
	 * The year as a number from [0..99].
	 *
	 * \bits 8
	 * \default 0
	 */
	int year{};

	/**
	 * The month as a number from [1..12].
	 *
	 * \bits 8
	 * \default 1
	 */
	int month{};

	/**
	 * The day as a number from [1..31].
	 *
	 * If the day does not exist, the date is set to the first day of the
	 * next month.
	 *
	 * \bits 8
	 * \default 1
	 */
	int day{};

	/**
	 * The weekday as a number from [0..6].
	 *
	 * \bits 3
	 * \default 0
	 */
	int weekday{};

	/**
	 * The hour as a number from [0..23] or [0..11].
	 *
	 * The valid range of the hour depends on the whether the RTC is
	 * configured to use 12 or 24-hour clock.
	 *
	 * \bits 8
	 * \default 0
	 */
	int hour{};

	/**
	 * The minute as a number from [0..59].
	 *
	 * \bits 8
	 * \default 0
	 */
	int minute{};

	/**
	 * The second as a number from [0..59].
	 *
	 * \bits 8
	 * \default 0
	 */
	int second{};

	/**
	 * Whether to use the 24-hour clock.
	 */
	bool use_24_hour_clock{};
};

/**
 * Information related to the execution of the NDS machine.
 *
 * The meaning of the fields change depending on whether it is used to
 * configure the execution (in), or reflect the result of the execution (out).
 */
struct nds_exec {
	/**
	 * The number of ARM9 cycles.
	 *
	 * \in the number of cycles to run for / target cycles to run until
	 * \out the number of cycles elapsed
	 */
	u64 cycles{};

	/**
	 * A pointer to the audio buffer.
	 *
	 * The audio buffer is stored as an array of signed 16 bit
	 * samples, at a sample rate of 32768 Hz.
	 *
	 * \in contains the microphone input (mono)
	 * \out contains the speaker output (stereo: interleaved left, right)
	 */
	s16 *audio_buf{};

	/**
	 * The length of the audio buffer in audio frames.
	 *
	 * An audio frame contains `n` samples, where `n` is the
	 * channel count.
	 */
	size_t audio_buf_len{};

	/**
	 * The framebuffer.
	 *
	 * The framebuffer is stored as an array of 32 bit pixels in
	 * ABGR32 format.
	 *
	 * The framebuffer has a width of 256 pixels, and a height of
	 * 384 pixels. The first half of the framebuffer contains the
	 * top screen, and the second half contains the bottom screen.
	 *
	 * \in ignored
	 * \out a pointer to the framebuffer
	 */
	u32 *fb{};

	/**
	 * The signal flags.
	 *
	 * The `SHUTDOWN` signal always suspends execution, regardless
	 * of whether it is specified.
	 *
	 * \in the signals to suspend execution on, if raised
	 * \out the signals that were raised, and thus suspended execution
	 */
	unsigned long sig_flags{};

	/**
	 * The emulated ARM9 / ARM7 CPU usage as a fraction from [0..1].
	 */
	std::pair<double, double> cpu_usage{};

	/**
	 * The emulated ARM9 / ARM7 DMA usage as a fraction from [0..1].
	 */
	std::pair<double, double> dma_usage{};
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
	nds_machine(const nds_config& cfg);
	~nds_machine();

	/**
	 * Load a file into the machine.
	 *
	 * \param pathname the file to load
	 * \param type the type of file
	 */
	void load_file(const std::filesystem::path& pathname, nds_file type);

	/**
	 * Unload a file from the machine.
	 *
	 * \param type the type of file
	 */
	void unload_file(nds_file type);

	/**
	 * Load a system file into the machine.
	 *
	 * \param pathname the file to load
	 * \param type the type of system file
	 */
	void load_system_file(
			const std::filesystem::path& pathname, nds_file type);

	/**
	 * Load a cartridge into the machine.
	 *
	 * If a cartridge is already loaded, it is ejected.
	 *
	 * \param pathname the file to load
	 */
	void load_cartridge(const std::filesystem::path& pathname);

	/**
	 * Eject the cartridge from the machine.
	 *
	 * If there is no cartridge this function does nothing.
	 * The currently loaded save file and set save type (if any)
	 * will be unloaded / reset.
	 */
	void eject_cartridge();

	/**
	 * Load an existing save file.
	 *
	 * \param pathname the file to load
	 */
	void load_savefile(const std::filesystem::path& pathname);

	/**
	 * Create a save file.
	 *
	 * If a save file already exists this function will fail.
	 *
	 * \param pathname the file to create
	 * \param savetype the save type
	 */
	void create_savefile(const std::filesystem::path& pathname,
			nds_savetype savetype);

	/**
	 * Automatically load a save file (based on the loaded
	 * cartridge).
	 *
	 * The pathname of the save file is determined from the
	 * pathname of the loaded cartridge, which is then used as if
	 * `load_savefile` was called.
	 */
	void autoload_savefile();

	/**
	 * Automatically create a save file (based on the loaded
	 * cartridge).
	 *
	 * The pathname is determined from the pathname of the loaded
	 * cartridge. If a save type is set, it is used to create the
	 * save file, otherwise, the save type is guessed from the
	 * cartridge.
	 *
	 */
	void autocreate_savefile();

	/**
	 * Query whether a file is loaded.
	 *
	 * \param type the type of file
	 * \returns whether the file is loaded
	 */
	bool file_loaded(nds_file type);

	/**
	 * Get the currently loaded files.
	 *
	 * \returns a mask of nds_file values
	 */
	int get_loaded_files();

	/**
	 * Sync files to disk.
	 */
	void sync_files();

	/**
	 * Get the currently set save type.
	 *
	 * \returns the current save type
	 */
	nds_savetype get_savetype();

	/**
	 * Set the save type of the cartridge.
	 *
	 * \param savetype the save type for the cartridge
	 */
	void set_savetype(nds_savetype savetype);

	/**
	 * Check the loaded save file.
	 *
	 * Checks whether the loaded save file has a size consistent
	 * with the save type to be used.
	 */
	void check_savefile();

	/**
	 * Prepare a save file for execution.
	 *
	 * If a save file is not loaded, it is automatically loaded or
	 * created.
	 */
	void prepare_savefile_for_execution();

	/**
	 * Boot up the machine.
	 *
	 * If the machine is already running, this function will do
	 * nothing.
	 *
	 * If the RTC state has not been set, the current time will
	 * automatically be used instead.
	 *
	 * If `direct_boot` is true, then a cartridge must already be
	 * loaded.
	 *
	 * \param direct_boot true to boot the cartridge directly,
	 *                    false to boot the firmware
	 */
	void boot(bool direct_boot);

	/**
	 * Shut down the machine.
	 *
	 * If the machine is already shut down, this function will do
	 * nothing.
	 *
	 * This function will always shut down the machine, but errors may
	 * occur during the shutdown process, e.g. the syncing of files.
	 *
	 * \returns 0 iff shutdown with no errors
	 */
	int shutdown() noexcept;

	/**
	 * Reboot the machine.
	 *
	 * This function behaves as if `shutdown` and `boot` were
	 * called.
	 *
	 * \param direct_boot true to boot the cartridge directly,
	 *                    false to boot the firmware
	 */
	void reboot(bool direct_boot);

	/**
	 * Run the machine until VBLANK.
	 *
	 * \param in the execution config
	 * \param out the execution result
	 */
	void run_until_vblank(const nds_exec *in, nds_exec *out);

	/**
	 * Check whether the machine is shut down.
	 *
	 * \returns true if the machine is shut down
	 *          false otherwise
	 */
	bool is_shutdown();

	/**
	 * Update the state of a button.
	 *
	 * \param button the button to update the state for
	 * \param down true if the button is pressed,
	 *             false if the button is released
	 */
	void update_button_state(nds_button button, bool down);

	/**
	 * Update the current touchscreen state.
	 *
	 * If `down` is false, then the x and y coordinates are
	 * ignored.
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
	 * Set the RTC state.
	 *
	 * If the machine is running, the state will be updated immediately.
	 * Otherwise, the state will be updated the next time the machine
	 * boots up.
	 *
	 * \param state the state
	 */
	void set_rtc_state(const nds_rtc_state& state);

	/**
	 * Set the RTC state to the current time.
	 */
	void set_rtc_state();

	/**
	 * Unset the RTC state.
	 */
	void unset_rtc_state();

	/**
	 * Set the audio mixing bit depth
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
	struct impl;
	std::unique_ptr<impl> m;
};

} // namespace twice

#endif
