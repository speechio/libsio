#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include <fstream>

#include "sio/error.h"
#include "sio/feature.h"
#include "sio/struct_loader.h"

namespace sio {
struct SpeechToTextConfig {
  bool online = true;

  std::string mean_var_norm_file;
  std::string tokenizer;
  std::string model;
  std::string graph;
  std::string context;

  bool do_endpointing = false;

  FeatureConfig feature;

  Error RegisterToLoader(StructLoader* loader, const std::string module = "") {
    loader->Register(module, ".online", &online);
    loader->Register(module, ".mean_var_norm_file", &mean_var_norm_file);
    loader->Register(module, ".tokenizer", &tokenizer);
    loader->Register(module, ".model", &model);
    loader->Register(module, ".graph", &graph);
    loader->Register(module, ".context", &context);
    loader->Register(module, ".do_endpointing", &do_endpointing);

    loader->Register(module + ".feature", ".type", &feature.feature_type);
    loader->Register(module + ".feature", ".fbank_config", &feature.fbank_config);

    return Error::OK;
  }

  Error Load(const std::string& config_file) {
    StructLoader loader;
    RegisterToLoader(&loader);

    Json json_config;
    std::ifstream config_stream(config_file);
    config_stream >> json_config;

    loader.Load(json_config);
    loader.Print();

    return Error::OK;
  }

}; // class SpeechToTextConfig
}  // namespace sio
#endif
