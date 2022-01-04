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
    const FeatureExtractorConfig& feature_extractor_config,
    const Tokenizer& tokenizer,
    const ScorerConfig& scorer_config, 
    torch::jit::script::Module& model
  ) :
    feature_extractor_(feature_extractor_config),
    tokenizer_(tokenizer),
    scorer_(tokenizer, scorer_config, model)
  { }

  Error Speech(const float* samples, size_t num_samples, float sample_rate) {
    feature_extractor_.PushAudio(samples, num_samples, sample_rate);
    while (feature_extractor_.NumFrames() > 0) {
      Vec<float> frame;
      feature_extractor_.PopFeat(&frame);
      scorer_.PushFeat(frame);
    }
    return Error::OK;
  }

  Error To() { 
    feature_extractor_.EndOfAudio();
    while (feature_extractor_.NumFrames() > 0) {
      Vec<float> frame;
      feature_extractor_.PopFeat(&frame);
      scorer_.PushFeat(frame);
    }    
    scorer_.EndOfFeats();
    return Error::OK;
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
  FeatureExtractor feature_extractor_;
  const Tokenizer& tokenizer_;
  Scorer scorer_;

}; // class Recognizer
}  // namespace sio
#endif
