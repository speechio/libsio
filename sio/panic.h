#ifndef SIO_PANIC_H
#define SIO_PANIC_H

#include "sio/error.h"
#include "sio/logger.h"

namespace sio {

class Panic {
public:
    Panic(const char* file, size_t line, const char* func, Error err) {
        fprintf(stderr, "[panic](%s:%d:%s) %s\n", file, line, func, ErrMsg(err));
        fflush(stderr);
    }

    ~Panic() { abort(); }
};

#define SIO_PANIC(err) ::sio::Panic(SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, err)

} // namespace sio

#endif

