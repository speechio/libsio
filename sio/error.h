#ifndef SIO_ERROR_H
#define SIO_ERROR_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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

const char* ErrorCStr(Error err);

} // namespace sio

#endif
