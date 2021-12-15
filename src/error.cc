#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "sio/error.h"

namespace sio {

const char *error_cstr(Error err) {
  switch (err) {
    case Error::None: return "(no error)";
    case Error::OOM: return "out of memory";
    case Error::Unreachable: return "control flow into unreachable";
    case Error::Precondition: return "pre-condition unsatisfied";
    case Error::Postcondition: return "post-condition unsatisfied";
    case Error::Invariant: return "invariant unsatisfied";
    case Error::Check: return "check failure";
    case Error::InvalidFileHandle: return "invalid file handle";
    case Error::Unknown: return "unknown error";
  }
  SIO_UNREACHABLE();
}


bool operator!(Error err) {
  return (err == Error::None);
}


void panic(const char* file, size_t line, const char* func, Error err) {
  fprintf(stderr, "panic: %s @ %s:%d:%s\n", error_cstr(err), file, line, func);
  fflush(stderr);
  abort();
}

} // namespace sio
