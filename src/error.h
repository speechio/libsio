#ifndef SIO_ERROR_H
#define SIO_ERROR_H

#include <stddef.h>

#include "sio/macro.h"

namespace sio {

enum class Error : int {
	OK = 0,
	OutOfMemory,
	Unreachable,
	AssertionFailure,
	InvalidFileHandle,
	Unknown,
}; // enum class Error

bool operator!(Error err);

const char *error_cstr(Error err);

bool error_is_fatal(Error err);

class Logger;
class Panic {
public:
	Panic(const char* file, size_t line, const char* func, Error err) :
		file_(file), line_(line), func_(func), err_(err) 
	{ }

	~Panic() {
		fprintf(stderr, "[panic](%s:%d:%s) %s\n", file_, line_, func_, error_cstr(err_));
		fflush(stderr);
		if (error_is_fatal(err_)) {
			abort();
		}
	}
	void operator&(const Logger &) { }

private:
	const char* file_;
	size_t line_;
	const char* func_;
	Error err_;
};

#define SIO_PANIC(err) ::sio::Panic(SIO__FILE__, __LINE__, SIO__FUNC__, err)

#define SIO_UNREACHABLE() SIO_PANIC(::sio::Error::Unreachable)

} // namespace sio

#endif
