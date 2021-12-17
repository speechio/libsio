#ifndef SIO_ERROR_H
#define SIO_ERROR_H

#include <cstddef>

#include "sio/base.h"

namespace sio {

enum class Error : int {
  kNone = 0,
  kOOM,
  kUnreachable,
  kAssertionFailure,
  kInvalidFileHandle,
  kUnknown,
}; // enum class Error

bool operator!(Error err);
const char *error_cstr(Error err);
bool fatal(Error err);

ABSL_ATTRIBUTE_NORETURN
void panic(const char* file, size_t line, const char* func, Error err);

#define SIO_PANIC(err) ::sio::panic(SIO_FILE_REPR, __LINE__, SIO_FUNC_REPR, err)

#define SIO_UNREACHABLE() SIO_PANIC(::sio::Error::kUnreachable)

} // namespace sio

#endif
