#include <gtest/gtest.h>
#include <iostream>

#include "sio/str.h"
#include "sio/type.h"
#include "sio/map.h"
#include "sio/audio.h"
#include "sio/feature.h"
#include "sio/mean_var_normalizer.h"

namespace sio {

TEST(Feature, ExtractorAndMeanVarNorm) {
  Map<Str, i32> audio_to_frames = {
      {"testdata/MINI/audio/audio1.wav", 126},
      {"testdata/MINI/audio/audio2.wav", 522}
  };

  float sample_rate = 16000;
  FeatureConfig c;
  c.feature_type = "fbank";
  FeatureInfo feature_info(c);

  MeanVarNormalizer mvn;
  mvn.Load("testdata/mean_var_normalizer_23dim.txt");

  for (const auto& kv : audio_to_frames) {
    Str audio_file = kv.first;
    i32 num_frames = kv.second;
    // TODO: can't move datapipe obj outside loop, need investigation
    FeatureExtractor feature_extractor(feature_info);

    std::ifstream is(audio_file, std::ifstream::binary);
    kaldi::WaveData wave;
    wave.Read(is);

    kaldi::SubVector<float> audio(wave.Data(), 0);
    feature_extractor.Forward(audio.Data(), audio.Dim(), sample_rate);
    feature_extractor.InputFinished();

    kaldi::Vector<float> frame_feat(feature_extractor.Dim());
    for (index_t f = 0; f < feature_extractor.NumFramesReady(); f++) {
      feature_extractor.GetFrame(f, &frame_feat);
      mvn.Forward(&frame_feat);
    }
    EXPECT_EQ(num_frames, feature_extractor.NumFramesReady());
  }

}

} // namespace sio