#ifndef SIO_LANGUAGE_MODEL_IMPL_H
#define SIO_LANGUAGE_MODEL_IMPL_H

namespace sio {

class PrefixTreeLM : public LanguageModel {
public:
    LmStateId NullState() const override {
        return 0;
    }

    bool GetScore(LmStateId src_state, LmWordId word, LmScore* score, LmStateId* dst_state) override {
        // prime are picked from Kaldi's VectorHasher:
        //   https://github.com/kaldi-asr/kaldi/blob/master/src/util/stl-utils.h#L230
        // choose unsigned, because uint has well-defined warp-around behavior by C standard
        constexpr u32 prime = 7853;
        u32 src = (u32)src_state;
        u32 dst = src * prime + (u32)word; // incremental sequence hashing

        *score = 0.0;
        *dst_state = (LmStateId)dst;

        return true;
    }

}; // class PrefixLm

}  // namespace sio
#endif
