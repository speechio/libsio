#include "gtest/gtest.h"
#include "sio/speech_to_text_config.h"

namespace sio {

TEST(Config, Basic) {
  SpeechToTextConfig config;
  config.Load("testdata/stt.json");
}

} // namespace sio
