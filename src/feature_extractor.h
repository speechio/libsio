#ifndef SIO_FEATURE_H
#define SIO_FEATURE_H

#include "feat/online-feature.h"

#include "sio/base.h"
#include "sio/struct_loader.h"
#include "sio/mean_var_norm.h"
namespace sio {

struct FeatureExtractorConfig {
  std::string type; // support "fbank" only for now
  kaldi::FbankOptions fbank;
  std::string mean_var_norm_file;

  Error Register(StructLoader* loader, const std::string module = "") {
    loader->AddEntry(module, ".type", &type);
    loader->AddEntry(module, ".sample_rate",  &fbank.frame_opts.samp_freq);
    loader->AddEntry(module, ".dither",       &fbank.frame_opts.dither);
    loader->AddEntry(module, ".num_mel_bins", &fbank.mel_opts.num_bins);
    loader->AddEntry(module, ".mean_var_norm_file", &mean_var_norm_file);
    return Error::OK;
  }
};

class FeatureExtractor {
 public:
  explicit FeatureExtractor(const FeatureExtractorConfig& config) :
    config_(config)
  { 
    SIO_CHECK_EQ(config_.type, "fbank");
    fbank_extractor_ = new kaldi::OnlineFbank(config_.fbank);

    if (config.mean_var_norm_file != "") {
      mean_var_norm_ = new MeanVarNorm(config.mean_var_norm_file);
    } else {
      mean_var_norm_ = nullptr;
    }
  }

  ~FeatureExtractor() {
    delete fbank_extractor_;
    if (mean_var_norm_) {
      delete mean_var_norm_;
    }
  }

  i32 Dim() {
    return fbank_extractor_->Dim();
  }

  void PushAudio(const float* samples, size_t num_samples, float sample_rate) {
    fbank_extractor_->AcceptWaveform(
      sample_rate, 
      kaldi::SubVector<float>(samples, num_samples)
    );
  }

  void EndOfAudio() {
    fbank_extractor_->InputFinished();
  }

  i32 NumFrames() const {
    return fbank_extractor_->NumFramesReady();
  }

  void GetFrame(i32 frame_idx, kaldi::VectorBase<float>* frame) {
    fbank_extractor_->GetFrame(frame_idx, frame);
    if (mean_var_norm_) {
      mean_var_norm_->Normalize(frame);
    }
  }

  void Reset() {
    SIO_CHECK_EQ(config_.type, "fbank");
    delete fbank_extractor_;
    fbank_extractor_ = new kaldi::OnlineFbank(config_.fbank);
  }

  float FrameShiftInSeconds() const {
    SIO_CHECK_EQ(config_.type, "fbank");
    return config_.fbank.frame_opts.frame_shift_ms / 1000.0f;
  }

 private:
  const FeatureExtractorConfig& config_;

  // need pointer here because we want Reset() functionality
  Optional<Owner<kaldi::OnlineBaseFeature*>> fbank_extractor_ = nullptr;
  Optional<Owner<MeanVarNorm*>> mean_var_norm_ = nullptr;
}; // class FeatureExtractor

}  // namespace sio
#endif
