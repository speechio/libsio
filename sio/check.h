#ifndef SIO_CHECK_H
#define SIO_CHECK_H

#include "sio/macro.h"
#include "sio/error.h"
#include "sio/logger.h"

namespace sio {

#define SIO_CHECK(cond) do {                       \
    if (SIO_UNLIKELY(!(cond))) {                   \
        SIO_FATAL << "Check {" #cond "} failed.";  \
        SIO_PANIC(::sio::Error::AssertionFailure); \
    }                                              \
} while(0) 


// CAUTION: operands evaluate more than once.
#define SIO_CHECK_PRED2(op, x, y) do {                  \
    if (SIO_UNLIKELY(!( (x) op (y) ))) {                \
        SIO_FATAL                                       \
            << "Check {" #x " " #op " " #y "} failed: " \
            << #x "=" << (x) << ", " #y "=" << (y);     \
        SIO_PANIC(::sio::Error::AssertionFailure);      \
    }                                                   \
} while(0)


#define SIO_CHECK_EQ(x, y)  SIO_CHECK_PRED2(==, x, y)
#define SIO_CHECK_NE(x, y)  SIO_CHECK_PRED2(!=, x, y)
#define SIO_CHECK_LT(x, y)  SIO_CHECK_PRED2(< , x, y)
#define SIO_CHECK_LE(x, y)  SIO_CHECK_PRED2(<=, x, y)
#define SIO_CHECK_GT(x, y)  SIO_CHECK_PRED2(> , x, y)
#define SIO_CHECK_GE(x, y)  SIO_CHECK_PRED2(>=, x, y)

} // namespace sio
#endif
