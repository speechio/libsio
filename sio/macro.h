#ifndef SIO_MACRO_H
#define SIO_MACRO_H

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
#define SIO_FILE_REPR  ::sio::Basename(__FILE__, sizeof(__FILE__) - 1)
#define SIO_LINE_REPR  __LINE__
#define SIO_FUNC_REPR  __func__


// https://github.com/kaldi-asr/kaldi/blob/dd107fd594ac58af962031c1689abfdc10f84452/src/base/kaldi-math.h#L67
// M_LN10
#define SIO_LN10 2.302585092994045684017991454684

} // namespace sio

#endif

