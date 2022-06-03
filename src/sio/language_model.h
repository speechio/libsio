#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/base.h"
#include "sio/json.h"
#include "sio/kenlm.h"
#include "sio/language_model_itf.h"
#include "sio/language_model_impl.h"

namespace sio {

enum class LmType : int {
    UndefinedLm,
    PrefixTreeLm,
    KenLm,
    FstLm
};

// Main purposes of this wrapper class:
// 1. expose polymorphic LM via value semantics instead of reference semantics
// 2. centralized LoadXXXLm() uses for typical LM types
class LanguageModel {
    Unique<LanguageModelItf*> pimpl_;
    bool major_ = false;

public:

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

    bool Major() { return major_; }

    LmStateId NullState() const {
        SIO_CHECK(pimpl_ != nullptr);
        return pimpl_->NullState();
    }


    inline LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate_ptr) {
        return pimpl_->GetScore(istate, word, ostate_ptr);
    }

}; // class LanguageModel


struct Context {
    bool major = false;

    str name;
    str path;

    f32 scale = 1.0;
    size_t cache = 100000;

    LmType type = LmType::UndefinedLm;
    Unique<KenLm*> kenlm;
}; // struct Context

} // namespace sio
#endif
