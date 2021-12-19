#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include "sio/error.h"
#include "sio/check.h"
#include "sio/ptr.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"

namespace sio {

template <typename BeamSearchGraphT>
class SpeechToText {
 private:
  SpeechToTextConfig config_;
  FeatureInfo feature_info_;

 public:
  SpeechToText(const SpeechToTextConfig& config) :
    config_(config),
    feature_info_(config_.feature_config)
  { }

  Optional<Recognizer*> CreateRecognizer() {
    try {
      return new Recognizer(feature_info_); 
    } catch (...) {
      return nullptr;
    }
  }

  void DestroyRecognizer(Recognizer* rec) {
    SIO_P_COND(rec != nullptr);
    delete rec;
  }
}; // class SpeechToText
}  // namespace sio

#endif
