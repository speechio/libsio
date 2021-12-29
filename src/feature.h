#ifndef SIO_FEATURE_H
#define SIO_FEATURE_H

#include "online2/online-nnet2-feature-pipeline.h"

#include "sio/base.h"
#include "sio/struct_loader.h"
#include "sio/mean_var_norm.h"
namespace sio {
struct FeatureExtractorConfig {
  kaldi::OnlineNnet2FeaturePipelineConfig kaldi;
  std::string mean_var_norm_file;

  Error Register(StructLoader* loader, const std::string module = "") {
    loader->AddEntry(module, ".type", &kaldi.feature_type);
    loader->AddEntry(module, ".fbank_config", &kaldi.fbank_config);
    loader->AddEntry(module, ".mean_var_norm_file", &mean_var_norm_file);
    return Error::OK;
  }
};

class FeatureExtractor {
 public:
  FeatureExtractor(const FeatureExtractorConfig& config) :
    info_(config.kaldi)
  { 
    kaldi_feature_pipeline_ = new kaldi::OnlineNnet2FeaturePipeline(info_);

    if (config.mean_var_norm_file != "") {
      mean_var_norm_ = new MeanVarNorm(config.mean_var_norm_file);
    }
  }

  ~FeatureExtractor() {
    delete kaldi_feature_pipeline_;
    if (mean_var_norm_) {
      delete mean_var_norm_;
    }
  }

  i32 FeatureDim() {
    return kaldi_feature_pipeline_->Dim();
  }

  void PushAudio(const float* samples, size_t num_samples, float sample_rate) {
    kaldi_feature_pipeline_->AcceptWaveform(
      sample_rate, 
      kaldi::SubVector<float>(samples, num_samples)
    );
  }

  void EndOfAudio() {
    kaldi_feature_pipeline_->InputFinished();
  }

  i32 NumFrames() const {
    return kaldi_feature_pipeline_->NumFramesReady();
  }

  void GetFrame(i32 frame_idx, kaldi::VectorBase<float>* frame) {
    kaldi_feature_pipeline_->GetFrame(frame_idx, frame);
    if (mean_var_norm_) {
      mean_var_norm_->Normalize(frame);
    }
  }

  void Reset() {
    delete kaldi_feature_pipeline_;
    kaldi_feature_pipeline_ = new kaldi::OnlineNnet2FeaturePipeline(info_);
  }

 private:
  kaldi::OnlineNnet2FeaturePipelineInfo info_;
  Owner<kaldi::OnlineNnet2FeaturePipeline*> kaldi_feature_pipeline_;
  Optional<Owner<MeanVarNorm*>> mean_var_norm_ = nullptr;
}; // class FeatureExtractor

}  // namespace sio
#endif
