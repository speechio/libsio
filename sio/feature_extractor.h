#ifndef SIO_FEATURE_H
#define SIO_FEATURE_H

#include <memory>

#include "feat/online-feature.h"

#include "sio/common.h"
#include "sio/struct_loader.h"
#include "sio/mean_var_norm.h"

namespace sio {
struct FeatureExtractorConfig {
    std::string type; // support "fbank" only for now
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

    // need pointer here to support fbank, mfcc etc
    Unique<kaldi::OnlineBaseFeature*> extractor_;

    Nullable<const MeanVarNorm*> mean_var_norm_ = nullptr;

    //[0, cur_frame_) popped frames, [cur_frame_, NumFramesReady()) remainder frames.
    index_t cur_frame_ = 0;


public:
    Error Load(const FeatureExtractorConfig& config, Nullable<const MeanVarNorm*> mvn = nullptr) { 
        SIO_CHECK_EQ(config.type, "fbank");
        config_ = &config;

        SIO_CHECK(!extractor_) << "Feature extractor initialized already.";
        extractor_ = make_unique<kaldi::OnlineFbank>(config_->fbank);

        mean_var_norm_ = mvn;

        cur_frame_ = 0;

        return Error::OK;
    }


    void Push(const f32* samples, size_t num_samples, f32 sample_rate) {
        extractor_->AcceptWaveform(
            sample_rate, 
            kaldi::SubVector<f32>(samples, num_samples)
        );
    }


    void PushEnd() {
        extractor_->InputFinished();
    }


    Vec<f32> Pop() {
        SIO_CHECK_GT(Size(), 0);
        Vec<f32> feat_frame(Dim(), 0.0f);

        // kaldi_frame is a helper frame view, no underlying data ownership
        kaldi::SubVector<f32> kaldi_frame(feat_frame.data(), feat_frame.size());
        extractor_->GetFrame(cur_frame_++, &kaldi_frame);
        if (mean_var_norm_) {
            mean_var_norm_->Normalize(&kaldi_frame);
        }

        return std::move(feat_frame);
    }


    Error Reset() {
        SIO_CHECK_EQ(config_->type, "fbank");
        extractor_.reset();
        extractor_ = make_unique<kaldi::OnlineFbank>(config_->fbank);
        cur_frame_ = 0;

        return Error::OK;
    }


    size_t Dim() const {
        return extractor_->Dim();
    }


    size_t Size() const {
        return extractor_->NumFramesReady() - cur_frame_;
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
