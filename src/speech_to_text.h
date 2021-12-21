#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include "sio/error.h"
#include "sio/check.h"
#include "sio/ptr.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"

namespace sio {

struct SpeechToText {
  SpeechToTextConfig config;
  Owner<FeatureInfo*> feature_info = nullptr;
  Optional<Owner<MeanVarNorm*>> mean_var_norm = nullptr;

 public:
  Error Load(std::string config_file) { 
    config.Load(config_file);

    // TODO: 
    // Get rid of this pointer, kaldi code doesn't provide trivial constructor
    // Consider switching to a value, read via a Load() function
    feature_info = new FeatureInfo(config.feature);

    if (config.mean_var_norm_file != "") {
      mean_var_norm = new MeanVarNorm;
      mean_var_norm->Load(config.mean_var_norm_file);
    }

    return Error::OK;
  }

  ~SpeechToText() {
    delete feature_info;
    delete mean_var_norm;
  }

  Optional<Recognizer*> CreateRecognizer() {
    try {
      return new Recognizer(*feature_info); 
    } catch (...) {
      return nullptr;
    }
  }

  void DestroyRecognizer(Recognizer* rec) {
    SIO_CHECK(rec != nullptr);
    delete rec;
  }
}; // class SpeechToText
}  // namespace sio

#endif
