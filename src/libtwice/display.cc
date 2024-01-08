#include "libtwice/nds/display.h"

#include <cmath>
#include <iostream>

namespace twice {

std::optional<std::pair<int, int>>
window_coords_to_screen_coords(double w, double h, double x, double y,
		bool letterboxed, int orientation, int gap)
{
	/* handle rotation */
	if (orientation & 1) {
		std::swap(w, h);
		std::swap(x, y);
		y = h - y;
	}

	if (orientation & 2) {
		x = w - x;
		y = h - y;
	}

	/* handle the gap */
	if (gap > 0) {
		double gap_start = (h - gap) / 2;
		double gap_end = gap_start + gap;

		h -= gap_start;

		if (gap_start < y && y < gap_end) {
			return {};
		} else if (y >= gap_end) {
			y -= gap;
		}
	}

	/* handle letterboxing */
	if (letterboxed) {
		double target_ratio = 256.0 / 384.0;
		double given_ratio = w / h;

		if (given_ratio == target_ratio) {
			x = x * 256.0 / w;
			y = (y - h / 2) * 192 / (h / 2);
		} else if (given_ratio <= target_ratio) {
			double new_h = w / target_ratio;
			x = x * 256.0 / w;
			y = (y - h / 2) * 192 / (new_h / 2);
		} else {
			double new_w = h * target_ratio;
			x = (x - (w - new_w) / 2) * 256 / new_w;
			y = (y - h / 2) * 192 / (h / 2);
		}
	} else {
		x = x * 256 / w;
		y = (y - h / 2) * 192 / (h / 2);
	}

	x = std::round(x);
	y = std::round(y);
	if (x < 0 || x >= 256 || y < 0 || y >= 192) {
		return {};
	} else {
		return { { x, y } };
	}
}

} // namespace twice
