#ifndef SIO_LANGUAGE_MODEL_IMPL_H
#define SIO_LANGUAGE_MODEL_IMPL_H

namespace sio {

class PrefixLm : public LanguageModel {
    // prefix word sequence is represented with a unique hash of type u64
    using PrefixUID = u32;

    Vec<PrefixUID> state_to_prefix_;
    Map<PrefixUID, LmStateId> prefix_to_state_;

public:

    Error Load() {
        // Establish null lm state: state index = 0, prefix hash = 0
        SIO_CHECK(state_to_prefix_.empty() && prefix_to_state_.empty());
        state_to_prefix_.push_back(0);
        prefix_to_state_[0] = 0;

        //dbg(prefix_to_state_);
        //dbg(state_to_prefix_);

        return Error::OK;
    }


    LmStateId NullState() const override {
        SIO_CHECK(!prefix_to_state_.empty()) << "PrefixLm uninitialzed ?";
        return 0;
    }


    bool GetScore(LmStateId src_state, LmWordId word, LmScore* score, LmStateId* dst_state) override {
        // prime are picked from Kaldi's VectorHasher:
        //   https://github.com/kaldi-asr/kaldi/blob/master/src/util/stl-utils.h#L230
        constexpr u32 prime = 7853;

        *score = 0.0;

        PrefixUID src = state_to_prefix_[src_state];
        PrefixUID dst = src * prime + (u32)word; // incremental sequence hashing

        auto it = prefix_to_state_.find(dst);
        if (it == prefix_to_state_.end()) {
            LmStateId s = state_to_prefix_.size();
            state_to_prefix_.push_back(dst);
            prefix_to_state_[dst] = s;

            *dst_state = s;
        } else {
            *dst_state = it->second;
        }

        return true;
    }

}; // class PrefixLm

}  // namespace sio
#endif
