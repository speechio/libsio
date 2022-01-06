#include <gtest/gtest.h>
#include <iostream>

#include "sio/common.h"
#include "sio/audio.h"
#include "sio/mean_var_norm.h"
#include "sio/feature_extractor.h"

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

    FeatureExtractor feature_extractor(config, nullptr); /* nullptr: no mvn */
    for (const auto& kv : audio_to_frames) {
        Str audio_file = kv.first;
        int num_frames = kv.second;

        std::vector<float> audio;
        float sample_rate;
        ReadAudio(audio_file, &audio, &sample_rate);

        feature_extractor.PushAudio(audio.data(), audio.size(), sample_rate);
        feature_extractor.EOS();
        EXPECT_EQ(num_frames, feature_extractor.NumFrames());

        Vec<float> frame;
        int i;
        while(feature_extractor.NumFrames() > 0) {
            feature_extractor.PopFeat(&frame);
        }
        feature_extractor.Reset();
    }
}

} // namespace sio
