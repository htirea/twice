#ifndef TWICE_EXCEPTION_H
#define TWICE_EXCEPTION_H

#include <exception>
#include <stdexcept>
#include <string>

namespace twice {

class TwiceException : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

class TwiceError : TwiceException {
	using TwiceException::TwiceException;
};

} // namespace twice

#endif
