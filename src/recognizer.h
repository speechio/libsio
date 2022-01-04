#ifndef SIO_RECOGNIZER_H
#define SIO_RECOGNIZER_H

#include <stddef.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "sio/base.h"
#include "sio/feature_extractor.h"
#include "sio/tokenizer.h"
#include "sio/scorer.h"

#include "sio/dbg.h"

namespace sio {
class Recognizer {
 public:
  Recognizer(
    const FeatureExtractorConfig& feature_extractor_config, const MeanVarNorm* mean_var_norm,
    const Tokenizer& tokenizer, const ScorerConfig& scorer_config, torch::jit::script::Module& nnet
  ) :
    feature_extractor_(feature_extractor_config, mean_var_norm),
    tokenizer_(tokenizer),
    scorer_(tokenizer, scorer_config, nnet)
  { }

  Error Speech(const float* samples, size_t num_samples, float sample_rate) {
    SIO_CHECK(samples != nullptr && num_samples != 0);
    return Proceed(samples, num_samples, sample_rate);
  }

  Error To() { 
    Error err = Proceed(nullptr, 0, 8000.16000 /*fake sample rate*/);
    return err;
  }

  Error Text(std::string* result) { 
    *result = scorer_.Result();
    return Error::OK;
  }

  Error Reset() { 
    feature_extractor_.Reset();
    scorer_.Reset();
    return Error::OK; 
  }

 private:
  Error Proceed(const float* samples, size_t num_samples, float sample_rate) {
    bool no_more_input = false;
    if (samples == nullptr && num_samples == 0) {
      no_more_input = true;
    }

    if (no_more_input) {
      feature_extractor_.EndOfAudio();
    } else {
      feature_extractor_.PushAudio(samples, num_samples, sample_rate);
    }

    while (feature_extractor_.NumFrames() > 0) {
      Vec<float> frame;
      feature_extractor_.PopFeat(&frame);
      scorer_.PushFeat(frame);
    }

    if (no_more_input) {
      scorer_.EndOfFeats();
    }

    return Error::OK;
  }

  FeatureExtractor feature_extractor_;
  const Tokenizer& tokenizer_;
  Scorer scorer_;

}; // class Recognizer
}  // namespace sio
#endif
