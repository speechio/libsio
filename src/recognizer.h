#ifndef SIO_RECOGNIZER_H
#define SIO_RECOGNIZER_H

#include <stddef.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "sio/common.h"
#include "sio/feature_extractor.h"
#include "sio/tokenizer.h"
#include "sio/scorer.h"
#include "sio/beam_search.h"

//#include "sio/dbg.h"

namespace sio {
class Recognizer {
private:
    const Tokenizer* tokenizer_ = SIO_UNDEF_PTR;
    FeatureExtractor feature_extractor_;
    Scorer scorer_;
    BeamSearch beam_search_;

public:

    Error Setup(
        const Tokenizer& tokenizer,
        const FeatureExtractorConfig& feature_extractor_config, const MeanVarNorm* mean_var_norm,
        const ScorerConfig& scorer_config, torch::jit::script::Module& nnet
    ) { 
        tokenizer_ = &tokenizer;
        feature_extractor_.Setup(feature_extractor_config, mean_var_norm);
        scorer_.Setup(scorer_config, nnet, feature_extractor_.Dim(), tokenizer_->Size());
        return Error::OK;
    }


    Error Reset() { 
        feature_extractor_.Reset();
        scorer_.Reset();
        beam_search_.Reset();
        return Error::OK; 
    }


    Error Speech(const float* samples, size_t num_samples, float sample_rate) {
        SIO_CHECK(samples != nullptr && num_samples != 0);
        return Advance(samples, num_samples, sample_rate, /*eos*/false);
    }


    Error To() { 
        Error err = Advance(nullptr, 0, /*dont care sample rate*/123.456, /*eos*/true);
        return err;
    }


    Error Text(std::string* result) { 
        auto best_path = beam_search_.BestPath();
        for (index_t i = 0; i < best_path.size(); i++) {
            *result += tokenizer_->Token(best_path[i]);
        }
        return Error::OK;
    }

private:

    Error Advance(const float* samples, size_t num_samples, float sample_rate, bool eos) {
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
            beam_search_.Push(score_frame);
        }
        
        if (eos) {
            beam_search_.PushEnd();
        }

        return Error::OK;
    }

}; // class Recognizer
}  // namespace sio
#endif
