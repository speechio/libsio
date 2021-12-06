#ifndef SIO_CHECK_H
#define SIO_CHECK_H

#include "sio/log.h"

#define SIO_LIKELY   ABSL_PREDICT_TRUE
#define SIO_UNLIKELY ABSL_PREDICT_FALSE

#define SIO_CHECK(expr, message) do {                           \
  if SIO_UNLIKELY(!(expr)) {                                    \
    SIO_ERROR << "Check {" << #expr << "} failed: " << message; \
  }                                                             \
} while(0)

#define P_COND(cond) SIO_CHECK(cond, "Precondition")
#define Q_COND(cond) SIO_CHECK(cond, "Postcondition")
#define INVAR(cond)  SIO_CHECK(cond, "Invariant")

#endif

