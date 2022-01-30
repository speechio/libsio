#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include <stddef.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "sio/common.h"
#include "sio/feature_extractor.h"
#include "sio/tokenizer.h"
#include "sio/scorer.h"
#include "sio/beam_search.h"
#include "sio/speech_to_text_model.h"

//#include "sio/dbg.h"

namespace sio {
class SpeechToText {
    const Tokenizer* tokenizer_ = nullptr;

    FeatureExtractor feature_extractor_;

    Scorer scorer_;

    Search search_;

public:

    Error Load(SpeechToTextModel& model) {
        SIO_CHECK(tokenizer_ == nullptr) << "Tokenizer initialized already.";
        tokenizer_ = &model.tokenizer;

        feature_extractor_.Load(
            model.config.feature_extractor, 
            model.mean_var_norm.get()
        );

        scorer_.Load(
            model.config.scorer,
            model.nnet,
            feature_extractor_.Dim(),
            tokenizer_->Size()
        );

        return Error::OK;
    }


    Error Speech(const f32* samples, size_t num_samples, f32 sample_rate) {
        SIO_CHECK(samples != nullptr && num_samples != 0);
        return Advance(samples, num_samples, sample_rate, /*eos*/false);
    }


    Error To() { 
        return Advance(nullptr, 0, /*dont care sample rate*/123.456, /*eos*/true);
    }


    Error Text(std::string* result) { 
        auto best_path = search_.BestPath();
        for (index_t i = 0; i < best_path.size(); i++) {
            *result += tokenizer_->Token(best_path[i]);
        }
        return Error::OK;
    }


    Error Reset() { 
        feature_extractor_.Reset();
        scorer_.Reset();
        search_.Reset();
        return Error::OK; 
    }

private:

    Error Advance(const f32* samples, size_t num_samples, f32 sample_rate, bool eos) {
        if (samples != nullptr && num_samples != 0) {
            feature_extractor_.Push(samples, num_samples, sample_rate);
        }
        if (eos) {
            feature_extractor_.PushEnd();
        }

        while (feature_extractor_.Len() > 0) {
            auto feat_frame = feature_extractor_.Pop();
            scorer_.Push(feat_frame);
        }
        if (eos) {
            scorer_.PushEnd();
        }

        while (scorer_.Len() > 0) {
            auto score_frame = scorer_.Pop();
            search_.Push(score_frame);
        }
        if (eos) {
            search_.PushEnd();
        }

        return Error::OK;
    }

}; // class SpeechToText
}  // namespace sio
#endif
