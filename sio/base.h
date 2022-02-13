#ifndef SIO_BASE_H
#define SIO_BASE_H

#include <string.h>

#include <absl/base/optimization.h>
#include <absl/base/attributes.h>

namespace sio {

/* branching prediction */
#define SIO_LIKELY   ABSL_PREDICT_TRUE
#define SIO_UNLIKELY ABSL_PREDICT_FALSE


/* file, line, func representation */
/*
#if defined(_MSC_VER)
#define SIO_FUNC_REPR __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__) || defined(__PRETTY_FUNCTION__)
#define SIO_FUNC_REPR __PRETTY_FUNCTION__
#else
#define SIO_FUNC_REPR __func__
#endif
*/
#define SIO_FUNC __func__

constexpr const char* Basename(const char* fname, int offset) {
    return offset == 0 || fname[offset - 1] == '/' || fname[offset - 1] == '\\'
               ? fname + offset
               : Basename(fname, offset - 1);
}
#define SIO_FILE  ::sio::Basename(__FILE__, sizeof(__FILE__) - 1)


/* "undefined" related stuff */
#define SIO_UNDEF_BYTE 0x00 /* Zig uses 0xAA */

template<typename T>
constexpr T MakeUndefValueOf() {
    T t;
    memset(&t, SIO_UNDEF_BYTE, sizeof(t));
    return t;
}
#define SIO_UNDEF_VAL(x) ::sio::MakeUndefValueOf<decltype(x)>()

#define SIO_UNDEFINED(x) ((x) == SIO_UNDEF_VAL(x))


/* delete macros */
template<typename T>
inline void Delete(T& p) {
    delete p;
    p = nullptr;
}

template <typename T>
inline void DeleteArray(T& p) {
    delete [] p;
    p = nullptr;
}

} // namespace sio
#endif

