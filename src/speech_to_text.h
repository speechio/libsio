#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include <torch/script.h>

#include "sio/error.h"
#include "sio/check.h"
#include "sio/ptr.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"

namespace sio {

struct SpeechToText {
  SpeechToTextConfig config;
  torch::jit::script::Module model;
  Optional<Owner<MeanVarNorm*>> mean_var_norm = nullptr;

 public:
  Error Load(std::string config_file) { 
    config.Load(config_file);

    if (config.mean_var_norm_file != "") {
      mean_var_norm = new MeanVarNorm;
      mean_var_norm->Load(config.mean_var_norm_file);
    }

    SIO_CHECK_NE(config.model, "") << "Need to provide a stt model.";
    SIO_INFO << "Loading torchscript model from: " << config.model;
    model = torch::jit::load(config.model);

    SIO_INFO << "subsampling_rate: " << model.run_method("subsampling_rate").toInt();
    SIO_INFO << "bidirectional decoder: " << model.run_method("is_bidirectional_decoder").toBool();
    SIO_INFO << "right context: " << model.run_method("right_context").toInt();

    return Error::OK;
  }

  ~SpeechToText() {
    delete mean_var_norm;
  }

  Optional<Recognizer*> CreateRecognizer() {
    try {
      return new Recognizer(
        config.feature,
        mean_var_norm,
        model
      ); 
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
