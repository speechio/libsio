#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include <fstream>

#include "sio/base.h"
#include "sio/feature.h"
#include "sio/struct_loader.h"

namespace sio {
struct SpeechToTextConfig {
  bool online = true;

  FeatureExtractorConfig feature_extractor;

  std::string tokenizer;
  std::string model;
  std::string graph;
  std::string context;
  bool do_endpointing = false;


  Error Register(StructLoader* loader, const std::string module = "") {
    loader->AddEntry(module, ".online", &online);

    feature_extractor.Register(loader, "feature_extractor");

    loader->AddEntry(module, ".tokenizer", &tokenizer);
    loader->AddEntry(module, ".model", &model);
    loader->AddEntry(module, ".graph", &graph);
    loader->AddEntry(module, ".context", &context);
    loader->AddEntry(module, ".do_endpointing", &do_endpointing);

    return Error::OK;
  }

  Error Load(const std::string& config_file) {
    StructLoader loader;
    Register(&loader);
    loader.Load(config_file);
    loader.Print();

    return Error::OK;
  }

}; // class SpeechToTextConfig
}  // namespace sio
#endif
