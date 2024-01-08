#ifndef LIBTWICE_NDS_DISPLAY_H
#define LIBTWICE_NDS_DISPLAY_H

#include <optional>
#include <utility>

namespace twice {

/**
 * Convert window coordinates to NDS screen coords.
 *
 * (0, 0) is the top left, x values increase going right, and y values increase
 * going down.
 *
 * The returned coordinates, if they exist, are rounded to the nearest integer.
 *
 * \param width the window width
 * \param height the window height
 * \param x the window x coordinate
 * \param y the window y coordinate
 * \param letterboxed whether the window is letterboxed
 * \param orientation 0 normal
 *                    1 rotated clockwise 90 degrees
 *                    2 rotated 180 degrees
 *                    3 rotated 270 degrees
 * \param gap the gap between the screens
 * \returns a pair containing the converted x and y coordinates
 */
std::optional<std::pair<int, int>> window_coords_to_screen_coords(double w,
		double h, double x, double y, bool letterboxed,
		int orientation, int gap);

} // namespace twice

#endif
