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

    LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate) override {
        // prime are picked from Kaldi's VectorHasher:
        //   https://github.com/kaldi-asr/kaldi/blob/master/istate/util/stl-utils.h#L230
        // choose unsigned, because uint has well-defined warp-around behavior by C standard
        constexpr u32 prime = 7853;
        *ostate = static_cast<LmStateId>((u32)istate * prime + (u32)word);

        return 0.0;
    }

}; // class PrefixTreeLm


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


    LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate) override {
        //SIO_CHECK(ostate != nullptr);

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
        *ostate = res.first->second;

        return score;
    }

}; // class NgramLm


class CacheLm : public LanguageModel {
    struct CacheK {
        LmStateId istate = -1; // -1 won't collide with any valid LmStateId
        LmWordId word;
    };

    struct CacheV {
        LmScore score;
        LmStateId ostate;
    };

    using Cache = std::pair<CacheK, CacheV>;

    LanguageModel* lm_ = nullptr;
    f32 scale_ = 1.0;
    Vec<Cache> caches_;

public:

    Error Load(LanguageModel& lm, f32 scale = 1.0, size_t cache_size = 100000) {
        SIO_CHECK_GT(cache_size, 0);

        SIO_CHECK(lm_ == nullptr);
        lm_ = &lm;

        scale_ = scale;

        SIO_CHECK(caches_.empty());
        caches_.resize(cache_size);

        return Error::OK;
    }


    LmStateId NullState() const override {
        return lm_->NullState();
    }


    LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate) override {
        Cache& cache = caches_[GetCacheIndex(istate, word)];
        CacheK& k = cache.first;
        CacheV& v = cache.second;

        if (k.istate != istate || k.word != word) { // cache miss
            k.istate = istate;
            k.word = word;

            v.score = scale_ * lm_->GetScore(istate, word, &v.ostate);
        }

        *ostate = v.ostate;
        return v.score;
    }

private:

    inline size_t GetCacheIndex(LmStateId istate, LmWordId word) {
        constexpr LmStateId p1 = 26597, p2 = 50329;
        return static_cast<size_t>(istate * p1 + word * p2) % caches_.size();
    }

}; // class CacheLm

}  // namespace sio
#endif
