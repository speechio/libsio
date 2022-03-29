#include "sio/feature_extractor.h"

#include <gtest/gtest.h>
#include <iostream>

#include "sio/base.h"
#include "sio/audio.h"
#include "sio/mean_var_norm.h"

namespace sio {

TEST(Feature, Extractor) {
    Map<Str, int> audio_to_frames = {
        {"testdata/MINI/audio/audio1.wav", 126},
        {"testdata/MINI/audio/audio2.wav", 522}
    };

    FeatureExtractorConfig config;
    config.type = "fbank";
    config.fbank.frame_opts.samp_freq = 16000;
    config.fbank.frame_opts.dither = 1.0;
    config.fbank.mel_opts.num_bins = 80;

    FeatureExtractor feature_extractor;
    feature_extractor.Load(config);
    for (const auto& kv : audio_to_frames) {
        Str audio_file = kv.first;
        int num_frames = kv.second;

        Vec<f32> audio;
        f32 sample_rate;
        ReadAudio(audio_file, &audio, &sample_rate);

        feature_extractor.Push(audio.data(), audio.size(), sample_rate);
        feature_extractor.PushEos();
        EXPECT_EQ(num_frames, feature_extractor.Size());

        while(feature_extractor.Size() > 0) {
            auto frame = feature_extractor.Pop();
        }
        feature_extractor.Reset();
    }
}

} // namespace sio
