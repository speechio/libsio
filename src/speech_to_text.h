#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include "sio/error.h"
#include "sio/ptr.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"

namespace sio {

struct SttStreamHandle {
  uint32_t recognizer = 0;
  const char* key = nullptr;
};

template <typename BeamSearchGraphT>
class SpeechToText {
 private:
  SpeechToTextConfig config_;
  FeatureInfo feature_info_;
  std::vector<Recognizer> recognizers_;

 public:
  SpeechToText(const SpeechToTextConfig& config) :
    config_(config),
    feature_info_(config_.feature_config)
  { }


  Error CreateStream(SttStreamHandle* stream, const char* stream_key) {
    SIO_P_COND(config_.max_threads == 1);
    SIO_P_COND(recognizers_.size() == 0);

    recognizers_.emplace_back(feature_info_);
    stream->key = stream_key;
    stream->recognizer = 0;

    Error err = recognizers_[stream->recognizer].Start(stream->key);
    SIO_CHECK(!err);
    return err;
  }


  Error SpeechIn(const SttStreamHandle& stream, const float* data, size_t len, float sample_rate) {
    SIO_P_COND(config_.max_threads == 1);
    SIO_P_COND(recognizers_.size() == 1);
    SIO_P_COND(stream.recognizer < recognizers_.size());

    Error err;
    err = recognizers_[stream.recognizer].Forward(data, len, sample_rate);
    SIO_CHECK(!err);

    return err;
  }


  Error TextOut(const SttStreamHandle& stream, std::string* text) {
    SIO_P_COND(config_.max_threads == 1);
    SIO_P_COND(recognizers_.size() == 1);
    SIO_P_COND(stream.recognizer < recognizers_.size());

    Error err;
    err = recognizers_[stream.recognizer].Stop();
    SIO_CHECK(!err);

    err = recognizers_[stream.recognizer].Result(text);
    SIO_CHECK(!err);

    return err;
  }


  Error DestroyStream(const SttStreamHandle& stream) { 
    SIO_P_COND(config_.max_threads == 1);
    SIO_P_COND(recognizers_.size() == 1);
    SIO_P_COND(stream.recognizer < recognizers_.size());
    recognizers_.pop_back();
    SIO_Q_COND(recognizers_.size() == 0);
    return Error::OK;
  }

}; // class SpeechToText
}  // namespace sio

#endif
