#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "sio/error.h"

namespace sio {

const char *error_cstr(Error err) {
  switch (err) {
    case ErrorNone: return "(no error)";
    case ErrorOOM: return "out of memory";
    case ErrorUnreachable: return "control flow into unreachable";
    case ErrorPrecondition: return "pre-condition unsatisfied";
    case ErrorPostcondition: return "post-condition unsatisfied";
    case ErrorInvariant: return "invariant unsatisfied";
    case ErrorCheck: return "check failure";
    case ErrorInvalidFileHandle: return "invalid file handle";
    case ErrorUnknown: return "unknown error";
  }
  SIO_UNREACHABLE();
}


void panic(const char* file, size_t line, const char* func, Error err) {
  fprintf(stderr, "panic: %s @ %s:%d:%s\n", error_cstr(err), file, line, func);
  fflush(stderr);
  abort();
}

} // namespace sio
