#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include "sio/feature.h"

namespace sio {
struct SpeechToTextConfig {
  FeatureConfig feature_config;

  std::string tokenizer;
  std::string model;
  std::string graph;
  std::string context;

  uint32_t num_workers = 1;

  bool do_endpointing = false;
  bool online = true;

  void Register(kaldi::OptionsItf *opts) {
    feature_config.Register(opts);

    opts->Register("tokenizer", &tokenizer, "sentencepiece tokenizer");
    opts->Register("model", &model, "nn model filename");
    opts->Register("graph", &graph, "beam search graph(e.g. HCLG.fst)");
    opts->Register("context", &context, "context configuration(e.g. context.json)");

    opts->Register("do-endpointing", &do_endpointing, "If true, apply endpoint detection");
  }
}; // class SpeechToTextConfig
}  // namespace sio
#endif
