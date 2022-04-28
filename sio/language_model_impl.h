#ifndef SIO_LANGUAGE_MODEL_IMPL_H
#define SIO_LANGUAGE_MODEL_IMPL_H

#include "sio/tokenizer.h"
#include "sio/kenlm.h"
#include "sio/language_model_itf.h"

namespace sio {

// PrefixTreeLm is used for LM-free decoding (e.g. vanilla CTC),
// Conceptually, it is a forever-expanding prefix tree:
// - each arc represents an emitted token / word
// - each leaf node represents a tree-search head
// - each path from root to leaf represents a unique decoded hypothesis
class PrefixTreeLm : public LanguageModelItf {
public:
    LmStateId NullState() const override {
        return 0;
    }

    LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate_ptr) override {
        // prime are picked from Kaldi's VectorHasher:
        //   https://github.com/kaldi-asr/kaldi/blob/master/istate/util/stl-utils.h#L230
        // choose unsigned, because uint has well-defined warp-around behavior by C standard
        constexpr u32 prime = 7853;
        *ostate_ptr = static_cast<LmStateId>((u32)istate * prime + (u32)word);

        return 0.0;
    }

}; // class PrefixTreeLm


// KenLm's model is designed to be stateless, so it can be shared by multiple threads.
// But query threads still need to handle KenLm's states, NgramLm severs as a state manager:
//   1. maintains an indexing system via a bidirectional map: state index <-> KenLm's state
//   2. provides index-based interface to hide actual states from outside world
// NgramLm instance is stateful so it should not be shared by multiple threads.
class NgramLm : public LanguageModelItf {
    // bidirectional map:
    //   state -> index via hashmap
    //   index -> state* via vector
    // Note that the hashmap implementation must not reallocate,
    // because reallocation will invalidate all pointers in the vector.
    // std::unordered_map is OK (backed with linked list), Google's swiss table is not.
    hashmap<KenLm::State, LmStateId, KenLm::StateHasher> state_to_index_;
    vec<const KenLm::State*> index_to_state_;

    const KenLm* kenlm_ = nullptr;

public:

    Error Load(const KenLm& kenlm) {
        SIO_CHECK(kenlm_ == nullptr);
        kenlm_ = &kenlm;

        KenLm::State null_state;
        kenlm_->SetStateToNull(&null_state);

        SIO_CHECK(state_to_index_.empty());
        // insert returns: std::pair<std::pair<KenLm::State, LmStateId>::iterator, bool>
        auto res = state_to_index_.insert({null_state, 0});
        SIO_CHECK(res.second == true);

        SIO_CHECK(index_to_state_.empty());
        index_to_state_.push_back(&(res.first->first));

        return Error::OK;
    }


    LmStateId NullState() const override {
        return 0;
    }


    LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate_ptr) override {
        //SIO_CHECK(ostate_ptr != nullptr);

        const KenLm::State* kenlm_istate = index_to_state_[istate];
        KenLm::State kenlm_ostate;
        LmScore score = kenlm_->Score(
            kenlm_istate,
            kenlm_->GetWordIndex(word),
            &kenlm_ostate
        );

        // insert returns: std::pair<std::pair<KenLm::State, LmStateId>::iterator, bool>
        auto res = state_to_index_.insert({kenlm_ostate, index_to_state_.size()});
        if (res.second) { // new elem inserted to the map
            index_to_state_.push_back(&(res.first->first));
        }
        *ostate_ptr = res.first->second;

        return score;
    }

}; // class NgramLm


class CachedLm : public LanguageModelItf {
    struct CacheK {
        LmStateId istate = -1; // -1 won't collide with any valid LmStateId
        LmWordId word;
    };

    struct CacheV {
        LmScore score;
        LmStateId ostate;
    };

    using Cache = std::pair<CacheK, CacheV>;

    Unique<LanguageModelItf*> lm_ = nullptr;
    f32 scale_ = 1.0;
    vec<Cache> caches_;

public:

    // NOTE sink argument: lm ownership transfered to loaded instance
    Error Load(Unique<LanguageModelItf*> lm, f32 scale, size_t cache_size) {
        SIO_CHECK(lm != nullptr);
        SIO_CHECK_GT(cache_size, 0);

        SIO_CHECK(lm_ == nullptr);
        lm_ = std::move(lm);

        scale_ = scale;

        SIO_CHECK(caches_.empty());
        caches_.resize(cache_size);

        return Error::OK;
    }


    LmStateId NullState() const override {
        return lm_->NullState();
    }


    LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate_ptr) override {
        Cache& cache = caches_[GetCacheIndex(istate, word)];
        CacheK& k = cache.first;
        CacheV& v = cache.second;

        if (k.istate != istate || k.word != word) { // cache miss
            k.istate = istate;
            k.word = word;

            v.score = scale_ * lm_->GetScore(istate, word, &v.ostate);
        }

        *ostate_ptr = v.ostate;
        return v.score;
    }

private:

    inline size_t GetCacheIndex(LmStateId istate, LmWordId word) {
        constexpr LmStateId p1 = 26597, p2 = 50329;
        return static_cast<size_t>(istate * p1 + word * p2) % caches_.size();
    }

}; // class CachedLm

}  // namespace sio
#endif
