#ifndef TWICE_EXCEPTION_H
#define TWICE_EXCEPTION_H

#include <exception>
#include <string>

namespace twice {

struct TwiceException : public std::exception {
	TwiceException(const std::string& msg)
		: msg(msg)
	{
	}

	virtual const char *what() const noexcept override
	{
		return msg.c_str();
	}

      private:
	std::string msg;
};

struct TwiceError : TwiceException {
	TwiceError(const std::string& msg)
		: TwiceException(msg)
	{
	}
};

struct FileError : TwiceException {
	FileError(const std::string& msg)
		: TwiceException(msg)
	{
	}
};

} // namespace twice

#endif
