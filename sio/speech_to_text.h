#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include <stddef.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "sio/common.h"
#include "sio/feature_extractor.h"
#include "sio/tokenizer.h"
#include "sio/scorer.h"
#include "sio/search.h"
#include "sio/speech_to_text_model.h"

//#include "sio/dbg.h"

namespace sio {
class SpeechToText {
    const Tokenizer* tokenizer_ = nullptr;

    FeatureExtractor feature_extractor_;

    Scorer scorer_;

    GreedySearch greedy_search_;
    BeamSearch beam_search_;

public:
    Error Load(SpeechToTextModel& model) {
        SIO_CHECK(tokenizer_ == nullptr) << "Tokenizer initialized already.";
        tokenizer_ = &model.tokenizer;

        SIO_INFO << "Loading feature extractor ...";
        feature_extractor_.Load(
            model.config.feature_extractor, 
            model.mean_var_norm.get()
        );

        SIO_INFO << "Loading scorer ...";
        scorer_.Load(
            model.config.scorer,
            model.nnet,
            feature_extractor_.Dim(),
            tokenizer_->Size()
        );

        SIO_INFO << "Loading beam search ...";
        beam_search_.Load(
            model.config.beam_search,
            model.graph,
            model.tokenizer
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
        auto greedy_best_path = greedy_search_.BestPath();
        auto beam_best_path = beam_search_.BestPath();

        for (const auto& token : greedy_best_path) {
            *result += tokenizer_->Token(token);
        }

        for (const auto& token : beam_best_path) {
            *result += tokenizer_->Token(token);
        }

        return Error::OK;
    }


    Error Reset() { 
        feature_extractor_.Reset();
        scorer_.Reset();

        greedy_search_.Reset();
        beam_search_.Reset();

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
            auto score_frame = scorer_.Pop();
            greedy_search_.Push(score_frame);
            beam_search_.Push(score_frame);
        }
        if (eos) {
            greedy_search_.PushEos();
            beam_search_.PushEos();
        }

        return Error::OK;
    }

}; // class SpeechToText
}  // namespace sio
#endif
