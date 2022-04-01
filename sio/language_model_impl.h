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
        if (res.second) { // new elem inserted to the map
            index_to_state_.push_back(&(res.first->first));
        }
        *dst = res.first->second;

        return score;
    }

}; // class NgramLm


class ScaleCacheLm : public LanguageModel {
    struct CacheK {
        LmStateId src = -1; // -1 won't collide with any valid LmStateId
        LmWordId word;
    }; 

    struct CacheV {
        LmScore score;
        LmStateId dst;
    };

    using CacheItem = std::pair<CacheK, CacheV>;

    LanguageModel* lm_ = nullptr;
    f32 scale_ = 1.0;
    Vec<CacheItem> cache_items_;

public:

    Error Load(LanguageModel& lm, f32 scale = 1.0, size_t cache_size = 100000) {
        lm_ = &lm;
        scale_ = scale;
        cache_items_.resize(cache_size);

        return Error::OK;
    }


    LmStateId NullState() const override {
        return 0;
    }


    LmScore GetScore(LmStateId src, LmWordId word, LmStateId* dst) override {
        CacheItem& item = cache_items_[GetCacheIndex(src, word)];
        CacheK& k = item.first;
        CacheV& v = item.second;

        if (k.src != src || k.word != word) { // cache miss
            k.src = src;
            k.word = word;

            v.score = scale_ * lm_->GetScore(src, word, &v.dst);
        }

        *dst = v.dst;
        return v.score;
    }

private:

    inline size_t GetCacheIndex(LmStateId src, LmWordId word) {
        constexpr LmStateId p1 = 26597, p2 = 50329;
        return static_cast<size_t>(src * p1 + word * p2) %
               static_cast<size_t>(cache_items_.size());
    }

}; // class ScaleCacheLm

}  // namespace sio
#endif
