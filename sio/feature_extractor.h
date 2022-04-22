#ifndef SIO_FEATURE_EXTRACTOR_H
#define SIO_FEATURE_EXTRACTOR_H

#include <memory>

#include "feat/online-feature.h"

#include "sio/base.h"
#include "sio/struct_loader.h"
#include "sio/mean_var_norm.h"

namespace sio {
struct FeatureExtractorConfig {
    std::string type; // support "fbank" only
    kaldi::FbankOptions fbank;

    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".type",         &type);
        loader->AddEntry(module + ".sample_rate",  &fbank.frame_opts.samp_freq);
        loader->AddEntry(module + ".dither",       &fbank.frame_opts.dither);
        loader->AddEntry(module + ".num_mel_bins", &fbank.mel_opts.num_bins);

        return Error::OK;
    }
};


class FeatureExtractor {
    const FeatureExtractorConfig* config_ = nullptr;

    Unique<kaldi::OnlineBaseFeature*> pimpl_; // polymorphic base pointer for fbank, mfcc etc

    Nullable<const MeanVarNorm*> mean_var_norm_ = nullptr; // MVN is optional

    // [0, cur_frame_) ~ popped frames
    // [cur_frame_, NumFramesReady()) ~ remainder frames.
    int cur_frame_ = 0;

public:

    Error Load(const FeatureExtractorConfig& config, Nullable<const MeanVarNorm*> mvn = nullptr) { 
        SIO_CHECK_EQ(config.type, "fbank");
        config_ = &config;

        SIO_CHECK(pimpl_ == nullptr);
        pimpl_ = std::make_unique<kaldi::OnlineFbank>(config.fbank);

        mean_var_norm_ = mvn;

        cur_frame_ = 0;

        return Error::OK;
    }


    void Push(const f32* samples, size_t num_samples, f32 sample_rate) {
        pimpl_->AcceptWaveform(
            sample_rate, 
            kaldi::SubVector<f32>(samples, num_samples)
        );
    }


    void PushEos() {
        pimpl_->InputFinished();
    }


    Vec<f32> Pop() {
        SIO_CHECK_GT(Size(), 0);
        Vec<f32> feat_frame(Dim(), 0.0f);

        // kaldi_frame is a helper frame view, no underlying data ownership
        kaldi::SubVector<f32> kaldi_frame(feat_frame.data(), feat_frame.size());
        pimpl_->GetFrame(cur_frame_, &kaldi_frame);
        if (mean_var_norm_) {
            mean_var_norm_->Normalize(&kaldi_frame);
        }
        cur_frame_++;

        return std::move(feat_frame);
    }


    Error Reset() {
        SIO_CHECK_EQ(config_->type, "fbank");
        pimpl_.reset();
        pimpl_ = std::make_unique<kaldi::OnlineFbank>(config_->fbank);
        cur_frame_ = 0;

        return Error::OK;
    }


    size_t Dim() const {
        return pimpl_->Dim();
    }


    size_t Size() const {
        return pimpl_->NumFramesReady() - cur_frame_;
    }


    f32 SampleRate() const {
        SIO_CHECK(config_ != nullptr);
        return config_->fbank.frame_opts.samp_freq;
    }


    f32 FrameRate() const {
        SIO_CHECK(config_ != nullptr);
        return 1000.0f / config_->fbank.frame_opts.frame_shift_ms;
    }

}; // class FeatureExtractor
}  // namespace sio
#endif
