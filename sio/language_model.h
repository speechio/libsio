#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/common.h"
#include "sio/tokenizer.h"

namespace sio {

using LmStateId = i32;
using LmWordId = i32;
using LmScore = f32;


class LanguageModel {
public:
    virtual LmWordId Bos() const = 0;
    virtual LmWordId Eos() const = 0;
    virtual LmWordId Unk() const = 0;

    virtual LmStateId NullLmState() const = 0;

    virtual bool GetLmScore(LmStateId src, LmWordId word, LmScore* score, LmStateId* dst) = 0;

    virtual ~LanguageModel() { }
};


class PrefixLm : public LanguageModel {
    using Prefix = size_t; // use a size_t hash to represent a prefix(lm state)

    static const int kPrime_ = 7853;
    static const LmWordId kNullLmStateId_ = 0;

    LmWordId bos_ = 0;
    LmWordId eos_ = 0;
    LmWordId unk_ = 0;

    Vec<Prefix> state_to_prefix_;
    Map<size_t, LmStateId> prefix_to_state_;

public:

    Error Load(const Tokenizer& tokenizer) {
        SIO_CHECK_EQ(state_to_prefix_.size(), 0);

        bos_ = tokenizer.bos;
        eos_ = tokenizer.eos;
        unk_ = tokenizer.unk;

        state_to_prefix_.push_back(kNullLmStateId_);
        return Error::OK;
    }


    LmWordId Bos() const override { return bos_; }
    LmWordId Eos() const override { return eos_; }
    LmWordId Unk() const override { return unk_; }


    LmStateId NullLmState() const override {
        SIO_CHECK_EQ(prefix_to_state_.at(0), kNullLmStateId_);
        return kNullLmStateId_;
    }


    bool GetLmScore(LmStateId src, LmWordId word, LmScore* score, LmStateId* dst) override {
        *score = 0.0;

        size_t src_prefix = state_to_prefix_[src];
        size_t dst_prefix = src_prefix * kPrime_ + word;

        auto r = prefix_to_state_.find(dst_prefix);
        if (r != prefix_to_state_.end()) {
            *dst = r->second;
        } else {
            *dst = state_to_prefix_.size();
            state_to_prefix_.push_back(dst_prefix);
        }

        return true;
    }

}; // class PrefixLm

} // namespace sio
#endif
