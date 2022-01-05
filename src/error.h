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
bool fatal(Error err);

void panic(const char* file, size_t line, const char* func, Error err);

#define SIO_PANIC(err) ::sio::panic(SIO_FILE_REPR, __LINE__, SIO_FUNC_REPR, err)

#define SIO_UNREACHABLE() SIO_PANIC(::sio::Error::Unreachable)

} // namespace sio

#endif
