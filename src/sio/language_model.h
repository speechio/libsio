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


struct Context {
    LmType type = LmType::UndefinedLm;

    str name;
    str path;

    bool major = false;
    f32 scale = 1.0;
    size_t cache = 10000;


    int Load(const Json& info) {
        if (info["type"] == "PrefixTreeLm") {
            type = LmType::PrefixTreeLm;

            name = info["name"];
            major = info["major"];

            return Error::OK;
        }

        if (info["type"] == "KenLm") {
            type = LmType::KenLm;

            name = info["name"];
            path = info["path"];
            major = info["major"];
            scale = info["scale"];
            cache = info["cache"];

            return Error::OK;
        }

        if (info["type"] == "FstLm") {
            type = LmType::FstLm;

            name = info["name"];
            path = info["path"];
            major = info["major"];
            scale = info["scale"];
            cache = info["cache"];

            return Error::OK;
        }

        SIO_PANIC(Error::Unreachable);
        return Error::OK;
    }
};


// main purposes of this wrapper class:
// 1. expose polymorphic LM via value semantics instead of reference semantics
// 2. centralized LoadXXXLm() uses for typical LM types
class LanguageModel {
    Unique<LanguageModelItf*> pimpl_;
    bool major_ = false;

public:

    Error LoadPrefixTreeLm(bool major = false) {
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

} // namespace sio
#endif
