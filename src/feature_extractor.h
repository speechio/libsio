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
    cur_frame_ = 0;

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

  size_t Dim() {
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

  size_t NumFrames() const {
    SIO_CHECK_LE(cur_frame_, fbank_extractor_->NumFramesReady());
    return fbank_extractor_->NumFramesReady() - cur_frame_;
  }

  index_t PopFeat(Vec<float>* frame) {
    SIO_CHECK_GT(NumFrames(), 0);

    frame->clear();
    frame->resize(Dim(), 0.0f);
    kaldi::SubVector<float> kframe(frame->data(), frame->size());

    index_t index_to_pop = cur_frame_;
    fbank_extractor_->GetFrame(cur_frame_++, &kframe);

    if (mean_var_norm_) {
      mean_var_norm_->Normalize(&kframe);
    }

    return index_to_pop;
  }

  void Reset() {
    SIO_CHECK_EQ(config_.type, "fbank");
    delete fbank_extractor_;
    fbank_extractor_ = new kaldi::OnlineFbank(config_.fbank);
    cur_frame_ = 0;
  }

  float FrameShiftInSeconds() const {
    SIO_CHECK_EQ(config_.type, "fbank");
    return config_.fbank.frame_opts.frame_shift_ms / 1000.0f;
  }

 private:
  const FeatureExtractorConfig& config_;

  // need pointer here because we want Reset() functionality
  Optional<Owner<kaldi::OnlineBaseFeature*>> fbank_extractor_ = nullptr;
  index_t cur_frame_; // [0, cur_frame_) + [cur_frame_, cur_frame_ + Frames()) = [0, NumFramesReady())

  // sometimes mvn is incoporated into nnet itself, so leave this nullptr
  Optional<Owner<MeanVarNorm*>> mean_var_norm_ = nullptr;
}; // class FeatureExtractor

}  // namespace sio
#endif
