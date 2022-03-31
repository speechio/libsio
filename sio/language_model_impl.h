#ifndef SIO_LANGUAGE_MODEL_IMPL_H
#define SIO_LANGUAGE_MODEL_IMPL_H

#include "sio/tokenizer.h"
#include "sio/kenlm.h"

namespace sio {

class PrefixTreeLm : public LanguageModel {
public:
    LmStateId NullState() const override {
        return 0;
    }

    LmScore GetScore(LmStateId src, LmWordId word, LmStateId* dst) override {
        // prime are picked from Kaldi's VectorHasher:
        //   https://github.com/kaldi-asr/kaldi/blob/master/src/util/stl-utils.h#L230
        // choose unsigned, because uint has well-defined warp-around behavior by C standard
        constexpr u32 prime = 7853;
        *dst = static_cast<LmStateId>((u32)src * prime + (u32)word);

        return 0.0;
    }

}; // class PrefixLM


/*
 * NgramLm severs as a state manager for KenLm model.
 *   1. KenLm model is stateless so it can be shared by multiple NgramLm instances.
 *   2. NgramLm instance is stateful so it cannot be shared by multiple threads.
 */
class NgramLm : public LanguageModel {
    // KenLm states are stored inside following hashmap.
    // Note that the hashmap implementation must not reallocate,
    // because that will invalidate all pointers in following indexing vector.
    // std::unordered_map uses linked list, which is OK, Google's swiss table is not.
    Map<KenLm::State, LmStateId, KenLm::StateHasher> state_to_index_;
    Vec<const KenLm::State*> index_to_state_;

    const KenLm* lm_ = nullptr;

public:

    Error Load(const KenLm& kenlm) {
        SIO_CHECK(lm_ == nullptr);
        lm_ = &kenlm;

        KenLm::State state;
        lm_->SetStateToNull(&state);

        SIO_CHECK(state_to_index_.empty());
        // insert returns: std::pair<std::pair<KenLm::State, LmStateId>::iterator, bool>
        auto res = state_to_index_.insert({state, 0});
        SIO_CHECK(res.second == true);

        SIO_CHECK(index_to_state_.empty());
        index_to_state_.push_back(&(res.first->first));

        return Error::OK;
    }


    LmStateId NullState() const override {
        return 0;
    }


    LmScore GetScore(LmStateId src, LmWordId word, LmStateId* dst) override {
        //SIO_CHECK_LT(src, static_cast<LmStateId>(index_to_state_.size()));

        KenLm::State dst_state;
        LmScore score = lm_->Score(
            index_to_state_[src], 
            lm_->GetWordIndex(word),
            &dst_state
        );

        // insert returns: std::pair<std::pair<KenLm::State, LmStateId>::iterator, bool>
        auto res = state_to_index_.insert({dst_state, index_to_state_.size()});
        if (res.second == true) {
            index_to_state_.push_back(&(res.first->first));
        }
        *dst = res.first->second;

        return score;
    }

}; // class LanguageModel

}  // namespace sio
#endif
