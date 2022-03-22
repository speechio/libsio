#ifndef SIO_CHECK_H
#define SIO_CHECK_H

#include "sio/base.h"
#include "sio/error.h"
#include "sio/logger.h"
#include "sio/panic.h"

namespace sio {

#define SIO_CHECK(cond) \
    SIO_LIKELY(cond) ? (void)0 : \
        SIO_PANIC(::sio::Error::AssertionFailure) & SIO_FATAL \
            << "Check {" #cond "} failed. "


// CAUTION: operands evaluate more than once.
#define SIO_CHECK_PRED2(op, x, y) \
    SIO_LIKELY( (x) op (y) ) ? (void)0 : \
        SIO_PANIC(::sio::Error::AssertionFailure) & SIO_FATAL \
            << "Check {" #x " " #op " " #y "} failed: " \
            << #x " ~> (" << (x) << "), " \
            << #y " ~> (" << (y) << "). "


#define SIO_CHECK_EQ(x, y)  SIO_CHECK_PRED2(==, x, y)
#define SIO_CHECK_NE(x, y)  SIO_CHECK_PRED2(!=, x, y)
#define SIO_CHECK_LT(x, y)  SIO_CHECK_PRED2(< , x, y)
#define SIO_CHECK_LE(x, y)  SIO_CHECK_PRED2(<=, x, y)
#define SIO_CHECK_GT(x, y)  SIO_CHECK_PRED2(> , x, y)
#define SIO_CHECK_GE(x, y)  SIO_CHECK_PRED2(>=, x, y)

} // namespace sio
#endif
