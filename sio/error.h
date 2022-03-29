#ifndef SIO_ERROR_H
#define SIO_ERROR_H

#include "sio/base.h"

namespace sio {

enum class Error : int {
    OK = 0,
    OutOfMemory,
    AssertionFailure,
    InvalidFileHandle,
    NoRecognitionResult,
    Unknown,
}; // enum class Error

const char* ErrorMsg(Error err);

void Panic(const char* file, size_t line, const char* func, Error err);

#define SIO_PANIC(err) ::sio::Panic(SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, err)

} // namespace sio

#endif

