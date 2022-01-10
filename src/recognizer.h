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
struct Recognizer {
    const Tokenizer* tokenizer = nullptr;
    FeatureExtractor feature_extractor;
    Scorer scorer;
    BeamSearch beam_search;


    Error Setup(
        const FeatureExtractorConfig& f, const MeanVarNorm* mvn,
        const Tokenizer& t,
        const ScorerConfig& s, torch::jit::script::Module& nnet
    ) {
        SIO_CHECK(tokenizer == nullptr) << "Tokenizer alrady initialized inside Setup().";
        feature_extractor.Setup(f, mvn);
        tokenizer = &t;
        scorer.Setup(s, nnet, feature_extractor.Dim(), tokenizer->Size());
        return Error::OK;
    }


    Error Reset() { 
        feature_extractor.Reset();
        scorer.Reset();
        beam_search.Reset();
        return Error::OK; 
    }


    Error Speech(const float* samples, size_t num_samples, float sample_rate) {
        SIO_CHECK(samples != nullptr && num_samples != 0);
        return Advance(samples, num_samples, sample_rate, /*eos*/false);
    }


    Error To() { 
        return Advance(nullptr, 0, /*dont care sample rate*/123.456, /*eos*/true);
    }


    Error Text(std::string* result) { 
        auto best_path = beam_search.BestPath();
        for (index_t i = 0; i < best_path.size(); i++) {
            *result += tokenizer->Token(best_path[i]);
        }
        return Error::OK;
    }

private:

    Error Advance(const float* samples, size_t num_samples, float sample_rate, bool eos) {
        if (samples != nullptr && num_samples != 0) {
            feature_extractor.Push(samples, num_samples, sample_rate);
        }

        if (eos) {
            feature_extractor.PushEnd();
        }

        while (feature_extractor.Len() > 0) {
            auto feat_frame = feature_extractor.Pop();
            scorer.Push(feat_frame);
        }

        if (eos) {
            scorer.PushEnd();
        }

        while (scorer.Len() > 0) {
            auto score_frame = scorer.Pop();
            beam_search.Push(score_frame);
        }
        
        if (eos) {
            beam_search.PushEnd();
        }

        return Error::OK;
    }

}; // class Recognizer
}  // namespace sio
#endif
