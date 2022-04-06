#include "sio/audio.h"

#include <gtest/gtest.h>
#include <string>

namespace sio {

TEST(Audio, Basic) {
    std::string audio_path = "testdata/MINI/audio/audio1.wav";
    Vec<f32> audio;
    f32 sample_rate = 0.0f;

    ReadAudio(audio_path, &audio, &sample_rate);
    EXPECT_EQ(audio.size(), 20480);
    EXPECT_EQ(sample_rate, 16000);
}

} // namespace sio
