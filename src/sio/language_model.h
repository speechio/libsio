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
    KenLm,
    FstLm
};


struct Context {
    LmType type = LmType::UndefinedLm;

    str name;
    str path;

    bool major = false;
    f32 scale = 1.0;
    size_t cache = 10000;


    int Load(const Json& info) {
        if (info["type"] == "KenLm") {
            type = LmType::KenLm;
        } else if (info["type"] == "FstLm") {
            type = LmType::FstLm;
        } else {
            SIO_PANIC(Error::Unreachable);
        }

        name = info["name"];
        path = info["path"];

        major = info["major"];
        scale = info["scale"];
        cache = info["cache"];

        return Error::OK;
    }
};


// main purposes of this wrapper class:
// 1. expose polymorphic LM via value semantics instead of reference semantics
// 2. centralized LoadXXXLm() uses for typical LM types
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


    inline LmScore GetScore(LmStateId istate, LmWordId word, LmStateId* ostate_ptr) {
        return pimpl_->GetScore(istate, word, ostate_ptr);
    }

}; // class LanguageModel

} // namespace sio
#endif
