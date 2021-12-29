#ifndef SIO_RECOGNIZER_H
#define SIO_RECOGNIZER_H

#include <stddef.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "sio/ptr.h"
#include "sio/check.h"
#include "sio/audio.h"
#include "sio/feature.h"
#include "sio/mean_var_norm.h"

namespace sio {
class Recognizer {
 public:
  Recognizer(
    const FeatureExtractorConfig& feature_extractor_config,
    torch::jit::script::Module& model
  ) :
    feature_extractor_(feature_extractor_config),
    model_(model)
  { }

  Error Speech(const float* data, size_t len, float sample_rate) {
    feature_extractor_.PushAudio(data, len, sample_rate);
    SIO_DEBUG << feature_extractor_.NumFrames();

    return Error::OK;
  }

  Error To() { return Error::OK; }

  Error Text(std::string* result) { 
    *result = "This is a recognition result.";
    return Error::OK;
  }

  Error Reset() { 
    feature_extractor_.Reset();
    return Error::OK; 
  }

 private:
  FeatureExtractor feature_extractor_;
  torch::jit::script::Module& model_;

}; // class Recognizer
}  // namespace sio
#endif
