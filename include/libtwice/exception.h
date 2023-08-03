#ifndef LIBTWICE_EXCEPTION_H
#define LIBTWICE_EXCEPTION_H

#include <exception>
#include <stdexcept>
#include <string>

namespace twice {

struct twice_exception : std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct twice_error : twice_exception {
	using twice_exception::twice_exception;
};

} // namespace twice

#endif
