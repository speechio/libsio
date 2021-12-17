#ifndef SIO_FEATURE_H
#define SIO_FEATURE_H

#include "online2/online-nnet2-feature-pipeline.h"
#include "sio/type.h"
#include "sio/log.h"

namespace sio {

using FeatureConfig = kaldi::OnlineNnet2FeaturePipelineConfig;
using FeatureInfo   = kaldi::OnlineNnet2FeaturePipelineInfo;

class FeatureExtractor {
 public:
  FeatureExtractor(const FeatureInfo& info) :
    extractor_(info)
  { }

  i32 FeatureDim() {
    return extractor_.Dim();
  }

  void PushAudio(const float* samples, size_t num_samples, float sample_rate) {
    extractor_.AcceptWaveform(
      sample_rate, 
      kaldi::SubVector<float>(samples, num_samples)
    );
  }

  i32 FramesReady() const {
    return extractor_.NumFramesReady();
  }

  void PushAudioDone() {
    extractor_.InputFinished();
  }

  void GetFrame(i32 frame_idx, kaldi::VectorBase<f32>* feat) {
    extractor_.GetFrame(frame_idx, feat);
  }

 private:
  kaldi::OnlineNnet2FeaturePipeline extractor_;
}; // class FeatureExtractor

}  // namespace sio
#endif
