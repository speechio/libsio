#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "sio/error.h"

namespace sio {

const char *error_cstr(Error err) {
  switch (err) {
    case Error::kNone: return "(no error)";
    case Error::kOOM: return "out of memory";
    case Error::kUnreachable: return "control flow into unreachable";
    case Error::kAssertionFailure: return "assertion failure";
    case Error::kInvalidFileHandle: return "invalid file handle";
    case Error::kUnknown: return "unknown error";
  }
  SIO_UNREACHABLE();
}

bool fatal(Error err) {
  return (static_cast<int>(err) > 0);
}

bool operator!(Error err) {
  return (err == Error::kNone);
}

void panic(const char* file, size_t line, const char* func, Error err) {
  fprintf(stderr, "[panic](%s:%d:%s) %s\n", file, line, func, error_cstr(err));
  fflush(stderr);
  if (fatal(err)) {
    abort();
  }
}

} // namespace sio
