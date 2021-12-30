#ifndef SIO_FEATURE_H
#define SIO_FEATURE_H

#include "online2/online-nnet2-feature-pipeline.h"

#include "sio/base.h"
#include "sio/struct_loader.h"
#include "sio/mean_var_norm.h"
namespace sio {

struct FeatureExtractorConfig {
  kaldi::OnlineNnet2FeaturePipelineConfig kaldi_feat_config;
  std::string mean_var_norm_file;

  Error Register(StructLoader* loader, const std::string module = "") {
    loader->AddEntry(module, ".type", &kaldi_feat_config.feature_type);
    loader->AddEntry(module, ".fbank_config", &kaldi_feat_config.fbank_config);
    loader->AddEntry(module, ".mean_var_norm_file", &mean_var_norm_file);
    return Error::OK;
  }
};

class FeatureExtractor {
 public:
  explicit FeatureExtractor(const FeatureExtractorConfig& config) :
    kaldi_feat_info_(config.kaldi_feat_config)
  { 
    kaldi_feat_pipe_ = new kaldi::OnlineNnet2FeaturePipeline(kaldi_feat_info_);

    if (config.mean_var_norm_file != "") {
      mean_var_norm_ = new MeanVarNorm(config.mean_var_norm_file);
    } else {
      mean_var_norm_ = nullptr;
    }
  }

  ~FeatureExtractor() {
    delete kaldi_feat_pipe_;
    if (mean_var_norm_) {
      delete mean_var_norm_;
    }
  }

  i32 FeatureDim() {
    return kaldi_feat_pipe_->Dim();
  }

  void PushAudio(const float* samples, size_t num_samples, float sample_rate) {
    kaldi_feat_pipe_->AcceptWaveform(
      sample_rate, 
      kaldi::SubVector<float>(samples, num_samples)
    );
  }

  void EndOfAudio() {
    kaldi_feat_pipe_->InputFinished();
  }

  i32 NumFrames() const {
    return kaldi_feat_pipe_->NumFramesReady();
  }

  void GetFrame(i32 frame_idx, kaldi::VectorBase<float>* frame) {
    kaldi_feat_pipe_->GetFrame(frame_idx, frame);
    if (mean_var_norm_) {
      mean_var_norm_->Normalize(frame);
    }
  }

  void Reset() {
    delete kaldi_feat_pipe_;
    kaldi_feat_pipe_ = new kaldi::OnlineNnet2FeaturePipeline(kaldi_feat_info_);
  }

 private:
  kaldi::OnlineNnet2FeaturePipelineInfo kaldi_feat_info_;
  // we need to use pointer here because kaldi class doesn't provide Reset() functionality
  Owner<kaldi::OnlineNnet2FeaturePipeline*> kaldi_feat_pipe_;
  Optional<Owner<MeanVarNorm*>> mean_var_norm_;
}; // class FeatureExtractor

}  // namespace sio
#endif
