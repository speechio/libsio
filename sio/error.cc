#include "sio/error.h"
namespace sio {

const char* ErrorCStr(Error err) {
    switch (err) {
        case Error::OK: return "(OK)";
        case Error::OutOfMemory: return "out of memory";
        case Error::AssertionFailure: return "assertion failure";
        case Error::InvalidFileHandle: return "invalid file handle";
        case Error::Unknown: return "(unknown error)";
    }
    return nullptr; /* avoid warning */
}

} // namespace sio

