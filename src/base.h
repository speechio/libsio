#ifndef SIO_BASE_H
#define SIO_BASE_H

#include <string.h>

#include <absl/base/optimization.h>
#include <absl/base/attributes.h>

namespace sio {

#define SIO_LIKELY   ABSL_PREDICT_TRUE
#define SIO_UNLIKELY ABSL_PREDICT_FALSE

/*
#if defined(_MSC_VER)
#define SIO_FUNC_REPR __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__) || defined(__PRETTY_FUNCTION__)
#define SIO_FUNC_REPR __PRETTY_FUNCTION__
#else
#define SIO_FUNC_REPR __func__
#endif
*/
#define SIO__FUNC__ __func__

constexpr const char* Basename(const char* fname, int offset) {
    return offset == 0 || fname[offset - 1] == '/' || fname[offset - 1] == '\\'
               ? fname + offset
               : Basename(fname, offset - 1);
}
#define SIO__FILE__  ::sio::Basename(__FILE__, sizeof(__FILE__) - 1)


//#define SIO_UNDEF_BYTE 0xAA  /* Zig uses 0xAA */
#define SIO_UNDEF_BYTE 0x00

template<typename T>
constexpr T make_undef_value() {
    T t;
    memset(&t, SIO_UNDEF_BYTE, sizeof(t));
    return t;
}
#define SIO_UNDEF_VALUE(x)  ::sio::make_undef_value<decltype(x)>()

#define SIO_UNDEFINED(x) ((x) == SIO_UNDEF_VALUE(x))

} // namespace sio
#endif

