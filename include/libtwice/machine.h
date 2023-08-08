#ifndef LIBTWICE_MACHINE_H
#define LIBTWICE_MACHINE_H

#include <cstddef>
#include <memory>
#include <string>

#include "libtwice/config.h"
#include "libtwice/filemap.h"

namespace twice {

struct nds_ctx;

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

	nds_machine(const nds_config& config);
	~nds_machine();

	void load_cartridge(const std::string& pathname);
	void boot(bool direct_boot);
	void run_frame();
	void *get_framebuffer();
	void button_event(nds_button button, bool down);

	/*
	 * Update the current touchscreen state
	 *
	 * x [0 - 255]
	 * y [0 - 191]
	 */
	void update_touchscreen_state(int x, int y, bool down);

	/*
	 * Update the current real world time used by the RTC.
	 *
	 * year: [2000 - 2099]
	 * month: [1 - 12]
	 * day: [1 - 31]
	 * weekday: [1 - 7] (Monday is 1)
	 * hour: [0 - 23]
	 * minute: [0 - 59]
	 * second: [0- 59]
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
