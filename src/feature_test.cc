#include <gtest/gtest.h>
#include <iostream>

#include "sio/str.h"
#include "sio/type.h"
#include "sio/map.h"
#include "sio/audio.h"
#include "sio/mean_var_norm.h"
#include "sio/feature.h"

namespace sio {

TEST(Feature, ExtractorAndMeanVarNorm) {
  Map<Str, int> audio_to_frames = {
      {"testdata/MINI/audio/audio1.wav", 126},
      {"testdata/MINI/audio/audio2.wav", 522}
  };

  float sample_rate = 16000;
  FeatureExtractorConfig config;
  config.kaldi.feature_type = "fbank";
  config.kaldi.fbank_config = "testdata/fbank.cfg";
  config.mean_var_norm_file = "testdata/mean_var_norm_80dim.txt";

  FeatureExtractor feature_extractor(config);
  for (const auto& kv : audio_to_frames) {
    Str audio_file = kv.first;
    int num_frames = kv.second;

    std::ifstream is(audio_file, std::ifstream::binary);
    kaldi::WaveData wave;
    wave.Read(is);

    kaldi::SubVector<float> audio(wave.Data(), 0);
    feature_extractor.PushAudio(audio.Data(), audio.Dim(), sample_rate);
    feature_extractor.EndOfAudio();
    EXPECT_EQ(num_frames, feature_extractor.NumFrames());

    kaldi::Vector<float> one_frame(feature_extractor.FeatureDim());
    for (int f = 0; f < feature_extractor.NumFrames(); f++) {
      feature_extractor.GetFrame(f, &one_frame);
    }
    feature_extractor.Reset();
  }
}

} // namespace sio
