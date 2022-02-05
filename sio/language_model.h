#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/common.h"

namespace sio {

using LmStateId = i32;
using LmWordId = i32;
using LmScore = f32;

class LanguageModel {
public:
    virtual LmWordId Bos() = 0;
    virtual LmWordId Eos() = 0;
    virtual LmWordId Unk() = 0;

    virtual LmStateId NullLmState() = 0;

    virtual bool GetLmScore(LmStateId src, LmWordId word, LmScore* score, LmStateId* dst) = 0;

    virtual ~LanguageModel() { }
};

} // namespace sio
#endif

