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


struct FeatureExtractor {
    const FeatureExtractorConfig* config = nullptr;

    // need pointer here to support fbank, mfcc etc
    Unique<kaldi::OnlineBaseFeature*> extractor;

    Nullable<const MeanVarNorm*> mean_var_norm = nullptr;

    // [0, cur_frame) ~ popped frames
    // [cur_frame, NumFramesReady()) ~ remainder frames
    index_t cur_frame = 0;


    Error Load(const FeatureExtractorConfig& config, Nullable<const MeanVarNorm*> mvn = nullptr) { 
        SIO_CHECK_EQ(config.type, "fbank");
        this->config = &config;

        SIO_CHECK(!this->extractor) << "Feature extractor initialized already.";
        this->extractor = make_unique<kaldi::OnlineFbank>(config.fbank);

        this->mean_var_norm = mvn;

        this->cur_frame = 0;

        return Error::OK;
    }


    void Push(const f32* samples, size_t num_samples, f32 sample_rate) {
        this->extractor->AcceptWaveform(
            sample_rate, 
            kaldi::SubVector<f32>(samples, num_samples)
        );
    }


    void PushEnd() {
        this->extractor->InputFinished();
    }


    Vec<f32> Pop() {
        SIO_CHECK_GT(Size(), 0);
        Vec<f32> feat_frame(Dim(), 0.0f);

        // kaldi_frame is a helper frame view, no underlying data ownership
        kaldi::SubVector<f32> kaldi_frame(feat_frame.data(), feat_frame.size());
        this->extractor->GetFrame(this->cur_frame, &kaldi_frame);
        if (this->mean_var_norm) {
            this->mean_var_norm->Normalize(&kaldi_frame);
        }
        this->cur_frame++;

        return std::move(feat_frame);
    }


    Error Reset() {
        SIO_CHECK_EQ(this->config->type, "fbank");
        this->extractor.reset();
        this->extractor = make_unique<kaldi::OnlineFbank>(this->config->fbank);
        this->cur_frame = 0;

        return Error::OK;
    }


    size_t Dim() const {
        return this->extractor->Dim();
    }


    size_t Size() const {
        return this->extractor->NumFramesReady() - this->cur_frame;
    }


    f32 SampleRate() const {
        SIO_CHECK(this->config != nullptr);
        return this->config->fbank.frame_opts.samp_freq;
    }


    f32 FrameRate() const {
        SIO_CHECK(this->config != nullptr);
        return 1000.0f / this->config->fbank.frame_opts.frame_shift_ms;
    }

}; // struct FeatureExtractor
}  // namespace sio
#endif
