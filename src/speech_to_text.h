#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include "sio/error.h"
#include "sio/check.h"
#include "sio/ptr.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"

namespace sio {

struct SttStreamHandle {
  uint32_t worker_id = 0;
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
    SIO_P_COND(config_.num_workers == 1);
    SIO_P_COND(recognizers_.size() == 0);

    recognizers_.emplace_back(feature_info_);
    stream->key = stream_key;
    stream->worker_id = 0;

    Error err = recognizers_[stream->worker_id].Reset(stream->key);
    SIO_CHECK(!err);
    return err;
  }


  Error SpeechIn(const SttStreamHandle& stream, const float* data, size_t len, float sample_rate) {
    SIO_P_COND(config_.num_workers == 1);
    SIO_P_COND(recognizers_.size() == 1);
    SIO_P_COND(stream.worker_id < recognizers_.size());

    Error err;
    err = recognizers_[stream.worker_id].Forward(data, len, sample_rate);
    SIO_CHECK(!err);

    return err;
  }


  Error PeekText(std::string* partial_text) {
    *partial_text = "partial result";
    return Error::OK;
  }


  Error TextOut(const SttStreamHandle& stream, std::string* text) {
    SIO_P_COND(config_.num_workers == 1);
    SIO_P_COND(recognizers_.size() == 1);
    SIO_P_COND(stream.worker_id < recognizers_.size());

    Recognizer& rec = recognizers_[stream.worker_id];

    Error err;
    err = rec.ForwardEnd();
    SIO_CHECK(!err);

    err = rec.Result(text);
    SIO_CHECK(!err);

    err = rec.Reset();
    SIO_CHECK(!err);

    return err;
  }


  Error DestroyStream(const SttStreamHandle& stream) { 
    SIO_P_COND(config_.num_workers == 1);
    SIO_P_COND(recognizers_.size() == 1);
    SIO_P_COND(stream.worker_id < recognizers_.size());
    recognizers_.pop_back();
    SIO_Q_COND(recognizers_.size() == 0);
    return Error::OK;
  }

}; // class SpeechToText
}  // namespace sio

#endif
