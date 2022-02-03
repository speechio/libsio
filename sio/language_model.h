#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/common.h"

namespace sio {

// virtual interface for generic language model in FST representation
using LmStateId = i32;
using WordId = i32;
using LmScore = f32;


struct LmArc {
    LmStateId src = 0;
    LmStateId dst = 0;
    WordId word = 0;
    LmScore score = 0.0;
};


class LanguageModel {
    virtual LmStateId Start() = 0;

    virtual bool GetLmArc(LmStateId src, WordId word, LmArc* arc) = 0;

    virtual ~LanguageModel() { }
};

} // namespace sio
#endif

