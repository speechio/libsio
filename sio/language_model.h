#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/base.h"

namespace sio {

using LmStateId = i32;
using LmWordId = i32;
using LmScore = f32;

class LanguageModel {
public:
    virtual LmStateId NullState() const = 0;

    virtual LmScore GetScore(LmStateId src, LmWordId word, LmStateId* dst) = 0;

    virtual ~LanguageModel() { }
};
} // namespace sio

#include "sio/language_model_impl.h"

#endif
