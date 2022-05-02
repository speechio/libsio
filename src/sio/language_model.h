#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/base.h"
#include "sio/kenlm.h"
#include "sio/language_model_itf.h"
#include "sio/language_model_impl.h"

namespace sio {

// main purposes of this wrapper class:
// 1. expose polymorphic LM through value semantics rather than reference semantics
// 2. centralized LoadXXXLm() uses for typical LM types
// 3. possible resource holders in addition to pimpl_
class LanguageModel {
    Unique<LanguageModelItf*> pimpl_;

public:

    Error LoadPrefixTreeLm() {
        SIO_CHECK(pimpl_ == nullptr);

        pimpl_ = std::make_unique<PrefixTreeLm>();
        return Error::OK;
    }


    Error LoadCachedNgramLm(const KenLm& kenlm, float scale = 1.0, size_t cache_size = 100000) {
        SIO_CHECK(pimpl_ == nullptr);

        Unique<NgramLm*> ngram_lm = std::make_unique<NgramLm>();
        ngram_lm->Load(kenlm);

        Unique<CachedLm*> cached_lm = std::make_unique<CachedLm>();
        cached_lm->Load(std::move(ngram_lm)/* sink */, scale, cache_size);

        pimpl_ = std::move(cached_lm);
        return Error::OK;
    }


    LmStateId NullState() const {
        SIO_CHECK(pimpl_ != nullptr);
        return pimpl_->NullState();
    }


    inline LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate_ptr) {
        return pimpl_->GetScore(istate, word, ostate_ptr);
    }

}; // class LanguageModel

} // namespace sio
#endif
