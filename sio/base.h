#ifndef SIO_BASE_H
#define SIO_BASE_H

#include <string.h>

#include <absl/base/optimization.h>
#include <absl/base/attributes.h>

namespace sio {

/* branching prediction */
#define SIO_LIKELY   ABSL_PREDICT_TRUE
#define SIO_UNLIKELY ABSL_PREDICT_FALSE


constexpr const char* Basename(const char* fname, int offset) {
    return offset == 0 || fname[offset - 1] == '/' || fname[offset - 1] == '\\'
               ? fname + offset
               : Basename(fname, offset - 1);
}
#define SIO_FILE  ::sio::Basename(__FILE__, sizeof(__FILE__) - 1)
#define SIO_FUNC __func__


/* safe delete */
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

