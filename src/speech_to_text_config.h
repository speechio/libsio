#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include <fstream>

#include "sio/error.h"
#include "sio/feature.h"
#include "sio/struct_loader.h"

namespace sio {
struct SpeechToTextConfig {
  bool online = true;

  FeatureConfig feature;
  std::string mean_var_norm_file;

  std::string tokenizer;
  std::string model;
  std::string graph;
  std::string context;
  bool do_endpointing = false;


  Error Register(StructLoader* loader, const std::string module = "") {
    loader->AddEntry(module, ".online", &online);

    loader->AddEntry(module + ".feature", ".type", &feature.feature_type);
    loader->AddEntry(module + ".feature", ".fbank_config", &feature.fbank_config);

    loader->AddEntry(module, ".mean_var_norm_file", &mean_var_norm_file);
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

    Json json_config;
    std::ifstream config_stream(config_file);
    if (config_stream.good()) {
      config_stream >> json_config;
    } else {
      SIO_FATAL(Error::InvalidFileHandle) << "Can't open stt config file: " << config_file;
    }

    loader.Load(json_config);
    loader.Print();

    return Error::OK;
  }

}; // class SpeechToTextConfig
}  // namespace sio
#endif
