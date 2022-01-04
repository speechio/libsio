#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include <torch/script.h>

#include "sio/base.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"
#include "sio/tokenizer.h"

namespace sio {

struct SpeechToText {
  SpeechToTextConfig config;
  torch::jit::script::Module model;
  Tokenizer tokenizer;

 public:
  Error Load(std::string config_file) { 
    config.Load(config_file);

    tokenizer.Load(config.tokenizer_vocab);
    dbg(tokenizer.index_to_token);
    dbg(tokenizer.token_to_index);

    SIO_CHECK(config.model != "") << "Need to provide a stt model.";
    SIO_INFO << "Loading torchscript model from: " << config.model;
    model = torch::jit::load(config.model);

    return Error::OK;
  }

  ~SpeechToText() { }

  Optional<Recognizer*> CreateRecognizer() {
    try {
      return new Recognizer(
        config.feature_extractor,
        tokenizer,
        config.scorer,
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
