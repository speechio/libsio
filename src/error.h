#ifndef SIO_ERROR_H
#define SIO_ERROR_H

#include <cstddef>

#include "sio/base.h"

namespace sio {

enum class Error : int {
  None = 0,
  OOM,
  Unreachable,
  Precondition,
  Postcondition,
  Invariant,
  Check,
  InvalidFileHandle,
  Unknown,
}; // enum class Error

bool operator!(Error err);
const char *error_cstr(Error err);


ABSL_ATTRIBUTE_NOINLINE
ABSL_ATTRIBUTE_NORETURN
void panic(const char* file, size_t line, const char* func, Error err);

#define SIO_PANIC(err) ::sio::panic(SIO_FILE_REPR, __LINE__, SIO_FUNC_REPR, err)

#define SIO_UNREACHABLE() SIO_PANIC(::sio::Error::Unreachable)

} // namespace sio

#endif
