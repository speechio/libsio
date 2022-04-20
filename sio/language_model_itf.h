#ifndef SIO_LANGUAGE_MODEL_ITF_H
#define SIO_LANGUAGE_MODEL_ITF_H

#include "sio/base.h"

namespace sio {

using LmStateId = i32;
using LmWordId = i32;
using LmScore = f32;

class LanguageModelItf {
public:
    virtual LmStateId NullState() const = 0;
    virtual LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate_ptr) = 0;
    virtual ~LanguageModelItf() { }
};

} // namespace sio
#endif
