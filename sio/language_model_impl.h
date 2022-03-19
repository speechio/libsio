#ifndef SIO_LANGUAGE_MODEL_IMPL_H
#define SIO_LANGUAGE_MODEL_IMPL_H

namespace sio {

class PrefixTreeLM : public LanguageModel {
    // prefix word sequence is represented with a unique hash of type u32
    using PrefixUID = u32;
public:

    LmStateId NullState() const override {
        return 0;
    }

    bool GetScore(LmStateId src_state, LmWordId word, LmScore* score, LmStateId* dst_state) override {
        // prime are picked from Kaldi's VectorHasher:
        //   https://github.com/kaldi-asr/kaldi/blob/master/src/util/stl-utils.h#L230
        constexpr u32 prime = 7853;
        PrefixUID src = (PrefixUID)src_state;
        PrefixUID dst = src * prime + (u32)word; // incremental sequence hashing

        *score = 0.0;
        *dst_state = (LmStateId)dst;

        return true;
    }

}; // class PrefixLm

}  // namespace sio
#endif
