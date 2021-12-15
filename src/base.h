#ifndef SIO_BASE_H
#define SIO_BASE_H

#include <absl/base/optimization.h>
#include <absl/base/attributes.h>

namespace sio {

#define SIO_LIKELY   ABSL_PREDICT_TRUE
#define SIO_UNLIKELY ABSL_PREDICT_FALSE

/* SIO_FUNC_REPR */
#define SIO_FUNC_REPR __func__
/*
#if defined(_MSC_VER)
#define SIO_FUNC_REPR __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__) || defined(__PRETTY_FUNCTION__)
#define SIO_FUNC_REPR __PRETTY_FUNCTION__
#else
#define SIO_FUNC_REPR __func__
#endif
*/

/* SIO_FILE_REPR */
constexpr const char* Basename(const char* fname, int offset) {
  return offset == 0 || fname[offset - 1] == '/' || fname[offset - 1] == '\\'
             ? fname + offset
             : Basename(fname, offset - 1);
}
#define SIO_FILE_REPR  ::sio::Basename(__FILE__, sizeof(__FILE__) - 1)

} // namespace sio
#endif
