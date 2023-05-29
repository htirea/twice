#ifndef TWICE_EXCEPTION_H
#define TWICE_EXCEPTION_H

#include <exception>
#include <stdexcept>
#include <string>

namespace twice {

struct TwiceException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct TwiceError : TwiceException {
	using TwiceException::TwiceException;
};

} // namespace twice

#endif
