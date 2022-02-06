#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/common.h"
#include "sio/tokenizer.h"
#include "sio/dbg.h"

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
    using Prefix = size_t; // use size_t as a unique hash to represent a prefix(lm state)

    const Tokenizer* tokenizer_ = nullptr;

    Vec<Prefix> state_to_prefix_;
    Map<Prefix, LmStateId> prefix_to_state_;

public:

    Error Load(const Tokenizer& tokenizer) {
        SIO_CHECK(tokenizer_ == nullptr);
        tokenizer_ = &tokenizer;

        // Establish null lm state: state index = 0, prefix hash = 0
        SIO_CHECK(state_to_prefix_.empty() && prefix_to_state_.empty());
        state_to_prefix_.push_back(0);
        prefix_to_state_[0] = 0;

        //dbg(prefix_to_state_);
        //dbg(state_to_prefix_);

        return Error::OK;
    }


    LmWordId Bos() const override { return tokenizer_->bos; }
    LmWordId Eos() const override { return tokenizer_->eos; }
    LmWordId Unk() const override { return tokenizer_->unk; }


    LmStateId NullLmState() const override {
        SIO_CHECK_EQ(prefix_to_state_.at(0), 0);
        return 0;
    }


    bool GetLmScore(LmStateId src_state, LmWordId word, LmScore* score, LmStateId* dst_state) override {
        // Vector hash from
        //   https://github.com/kaldi-asr/kaldi/blob/master/src/util/stl-utils.h#L230
        const int prime = 7853;

        *score = 0.0;

        size_t src_prefix = state_to_prefix_[src_state];
        size_t dst_prefix = src_prefix * prime + word;

        auto r = prefix_to_state_.find(dst_prefix);
        if (r != prefix_to_state_.end()) {
            *dst_state = r->second;

        } else {
            LmStateId new_state = state_to_prefix_.size();
            state_to_prefix_.push_back(dst_prefix);
            prefix_to_state_[dst_prefix] = new_state;

            *dst_state = new_state;
        }

        return true;
    }

}; // class PrefixLm

} // namespace sio
#endif
