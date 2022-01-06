#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "sio/error.h"
namespace sio {

const char *error_cstr(Error err) {
	switch (err) {
		case Error::OK: return "(OK)";
		case Error::OutOfMemory: return "out of memory";
		case Error::Unreachable: return "control flow into unreachable";
		case Error::AssertionFailure: return "assertion failure";
		case Error::InvalidFileHandle: return "invalid file handle";
		case Error::Unknown: return "(unknown error)";
	}
	SIO_UNREACHABLE();
}

bool error_is_fatal(Error err) {
	return (static_cast<int>(err) > 0);
}

bool operator!(Error err) {
	return (err == Error::OK);
}

} // namespace sio
