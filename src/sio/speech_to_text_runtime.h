#ifndef SIO_SPEECH_TO_TEXT_RUNTIME_H
#define SIO_SPEECH_TO_TEXT_RUNTIME_H

#include <stddef.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "sio/base.h"
#include "sio/feature_extractor.h"
#include "sio/tokenizer.h"
#include "sio/scorer.h"
#include "sio/beam_search.h"
#include "sio/speech_to_text_module.h"

namespace sio {
class SpeechToTextRuntime {
    const Tokenizer* tokenizer_ = nullptr;
    FeatureExtractor feature_extractor_;
    Scorer scorer_;
    BeamSearch beam_search_;
    str text_;

public:

    Error Load(SpeechToTextModule& m) {
        SIO_CHECK(tokenizer_ == nullptr); // Can't reload
        tokenizer_ = &m.tokenizer;

        SIO_INFO << "Loading feature extractor ...";
        feature_extractor_.Load(
            m.config.feature_extractor, 
            m.mean_var_norm.get()
        );

        SIO_INFO << "Loading scorer ...";
        scorer_.Load(
            m.config.scorer,
            m.nnet,
            feature_extractor_.Dim(),
            tokenizer_->Size()
        );

        SIO_INFO << "Loading beam search ...";
        beam_search_.Load(
            m.config.beam_search,
            m.graph,
            m.tokenizer
        );

        return Error::OK;
    }


    Error Speech(const f32* samples, size_t num_samples, f32 sample_rate) {
        SIO_CHECK(samples != nullptr && num_samples != 0);
        return Advance(samples, num_samples, sample_rate, /*eos*/false);
    }


    Error To() { 
        Advance(nullptr, 0, /*dont care sample rate*/123.456, /*eos*/true);

        for (const vec<TokenId>& path : beam_search_.NBest()) {
            for (const auto& t : path) {
                text_ += tokenizer_->Token(t);
            }
            text_ += "\t";
        }
        
        return Error::OK;
    }


    const char* Text() const {
        return text_.c_str();
    }


    Error Reset() { 
        feature_extractor_.Reset();
        scorer_.Reset();
        beam_search_.Reset();
        text_.clear();

        return Error::OK; 
    }

private:

    Error Advance(const f32* samples, size_t num_samples, f32 sample_rate, bool eos) {
        if (samples != nullptr && num_samples != 0) {
            feature_extractor_.Push(samples, num_samples, sample_rate);
        }
        if (eos) {
            feature_extractor_.PushEos();
        }

        while (feature_extractor_.Size() > 0) {
            scorer_.Push(feature_extractor_.Pop());
        }
        if (eos) {
            scorer_.PushEos();
        }

        while (scorer_.Size() > 0) {
            beam_search_.Push(scorer_.Pop());
        }
        if (eos) {
            beam_search_.PushEos();
        }

        return Error::OK;
    }

}; // class SpeechToTextRuntime
}  // namespace sio
#endif
