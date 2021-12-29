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
    FeatureConfig& feature_config,
    Optional<const MeanVarNorm*> mean_var_norm,
    torch::jit::script::Module& model
  ) :
    feature_info_(feature_config),
    feature_extractor_(feature_info_),
    mean_var_norm_(mean_var_norm),
    model_(model)
  { }

  Error Speech(const float* data, size_t len, float sample_rate) {
    AudioSegment<const float> audio_seg(data, len, sample_rate);
    feature_extractor_.PushAudio(
      audio_seg.data, audio_seg.len, audio_seg.sample_rate
    );
    SIO_DEBUG << feature_extractor_.NumFrames();

    return Error::OK;
  }

  Error To() { return Error::OK; }

  Error Text(std::string* result) { 
    *result = "This is a recognition result.";
    return Error::OK;
  }

  Error Reset(const char* key = nullptr) { return Error::OK; }

 private:
  FeatureInfo feature_info_;
  FeatureExtractor feature_extractor_;
  Optional<const MeanVarNorm*> mean_var_norm_;

  torch::jit::script::Module& model_;

}; // class Recognizer
}  // namespace sio
#endif
