#ifndef SIO_HOARE_H
#define SIO_HOARE_H

#include <stdio.h>

#if defined(__clang__) || defined(__GNUC__)
#    define LIKELY(x)   __builtin_expect(!!(x), 1)
#    define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#    define LIKELY(x)   (!!(x))
#    define UNLIKELY(x) (!!(x))
#endif

#define SIO_CHECK_EXPR(expr, info) do {                        \
    if (UNLIKELY( !(expr) )) {                                 \
        fprintf(stderr,                                        \
            "%s -> { %s } failed @ %s:%s:%d\n",               \
            (info) , (#expr), __FILE__, __FUNCTION__, __LINE__ \
        );                                                     \
        abort();                                               \
    }                                                          \
} while(0)

#define P_COND(cond)    SIO_CHECK_EXPR(cond, "Precondition")
#define Q_COND(cond)    SIO_CHECK_EXPR(cond, "Postcondition")
#define INVARIANT(cond) SIO_CHECK_EXPR(cond, "Invariant")

#endif
