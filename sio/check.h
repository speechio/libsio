#ifndef SIO_HOARE_H
#define SIO_HOARE_H

#include <stdio.h>
#include "absl/base/internal/pretty_function.h"
#include "absl/base/optimization.h"

#define SIO_CHECK(expr, message) do {                                     \
    if (ABSL_PREDICT_FALSE(!(expr))) {                                    \
        fprintf(stderr,                                                   \
            "%s -> Check { %s } failed @ %s:%s:%d\n",                     \
            (message) , (#expr), __FILE__, ABSL_PRETTY_FUNCTION, __LINE__ \
        );                                                                \
        abort();                                                          \
    }                                                                     \
} while(0)

#define P_COND(cond)    SIO_CHECK(cond, "Precondition")
#define Q_COND(cond)    SIO_CHECK(cond, "Postcondition")
#define INVARIANT(cond) SIO_CHECK(cond, "Invariant")

#endif
