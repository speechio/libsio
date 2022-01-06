#include <gtest/gtest.h>
#include <iostream>
#include <string>

#include "sio/audio.h"

namespace sio {

/*
TEST(Audio, LoaderAndResampler) {

    std::string audio_path = "testdata/MINI/audio/audio1.wav";
    std::ifstream is(audio_path, std::ifstream::binary);
    kaldi::WaveData wave_data;
    wave_data.Read(is);
    kaldi::SubVector<float> audio(wave_data.Data(), 0);

    Resampler resampler(16000, 8000);
    EXPECT_EQ(wave_data.SampFreq(), 16000.0);
    EXPECT_EQ(audio.Dim(), 20480);

    kaldi::Vector<float> output;
    resampler.Forward(audio.Data(), audio.Dim(), wave_data.SampFreq(), &output, true);

    EXPECT_EQ(resampler.TargetSampleRate(), 8000.0);
    EXPECT_EQ(output.Dim(), 10240);
}
*/

} // namespace sio
