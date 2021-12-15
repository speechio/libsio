#ifndef SIO_CHECK_H
#define SIO_CHECK_H

#include "sio/base.h"
#include "sio/log.h"
#include "sio/error.h"

#define SIO_CHECK(expr, err_msg, err) do {                  \
  if SIO_UNLIKELY(!(expr)) {                                \
    SIO_FATAL << "{" << #expr << "} -> false. " << err_msg; \
    SIO_PANIC(err);                                         \
  }                                                         \
} while(0)

#define SIO_P_COND(cond) SIO_CHECK(cond, "", ::sio::ErrorPrecondition)
#define SIO_Q_COND(cond) SIO_CHECK(cond, "", ::sio::ErrorPostcondition)
#define SIO_INVAR(cond)  SIO_CHECK(cond, "", ::sio::ErrorInvariant)


#endif

