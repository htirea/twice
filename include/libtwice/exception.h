#ifndef LIBTWICE_EXCEPTION_H
#define LIBTWICE_EXCEPTION_H

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

} // namespace twice

#endif
