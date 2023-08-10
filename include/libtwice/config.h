#ifndef LIBTWICE_CONFIG_H
#define LIBTWICE_CONFIG_H

#include <string>

namespace twice {

/**
 * Set the verbosity level of the log messages.
 *
 * The default level is set to 0. Increase the level to increase the verbosity.
 * A level of -1 or less will silence log messages.
 *
 * \param level the verbosity level
 */
void set_logger_verbose_level(int level);

} // namespace twice

#endif
