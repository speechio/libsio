#ifndef SIO_MACRO_H
#define SIO_MACRO_H

#include <string.h>

#include <absl/base/optimization.h>
#include <absl/base/attributes.h>

namespace sio {

/* compiler branching prediction hints */
#define SIO_LIKELY   ABSL_PREDICT_TRUE
#define SIO_UNLIKELY ABSL_PREDICT_FALSE

constexpr const char* Basename(const char* fname, int offset) {
    return offset == 0 || fname[offset - 1] == '/' || fname[offset - 1] == '\\'
               ? fname + offset
               : Basename(fname, offset - 1);
}
#define SIO_FILE_REPR  ::sio::Basename(__FILE__, sizeof(__FILE__) - 1)
#define SIO_LINE_REPR  __LINE__
#define SIO_FUNC_REPR  __func__

} // namespace sio

#endif

