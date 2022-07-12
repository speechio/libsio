#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/base.h"
#include "sio/json.h"
#include "sio/kenlm.h"
#include "sio/language_model_impl.h"

namespace sio {

// Main purposes of this wrapper class:
// 1. expose polymorphic LM via value semantics instead of reference semantics
// 2. centralized LoadXXXLm() uses for typical LM types
class LanguageModel {
    Unique<LmInterface*> pimpl_;
    bool major_ = false;

public:
    bool Major() { return major_; }

    Error LoadPrefixTreeLm(bool major = true) {
        SIO_CHECK(pimpl_ == nullptr);

        major_ = major;
        pimpl_ = std::make_unique<PrefixTreeLm>();

        return Error::OK;
    }


    Error LoadCachedNgramLm(const KenLm& kenlm, float scale = 1.0, size_t cache_size = 100000, bool major = false) {
        SIO_CHECK(pimpl_ == nullptr);

        major_ = major;

        Unique<NgramLm*> ngram = std::make_unique<NgramLm>();
        ngram->Load(kenlm);

        Unique<CachedLm*> cached_ngram = std::make_unique<CachedLm>();
        cached_ngram->Load(std::move(ngram)/*sink*/, scale, cache_size);

        pimpl_ = std::move(cached_ngram);

        return Error::OK;
    }


    LmStateId NullState() const {
        SIO_CHECK(pimpl_ != nullptr);
        return pimpl_->NullState();
    }


    inline f32 GetScore(LmStateId istate, TokenId token, LmStateId* ostate_ptr) {
        return pimpl_->GetScore(istate, token, ostate_ptr);
    }

}; // class LanguageModel


struct Context {
    str name;
    str tags;

    str path;

    f32 scale = 0.0;
    size_t cache = 0;

    LmType type = LmType::UndefinedLm;
    Unique<KenLm*> kenlm;
}; // struct Context

} // namespace sio
#endif
