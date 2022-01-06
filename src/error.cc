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

} // namespace sio

