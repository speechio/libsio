#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include "sio/feature.h"
#include "sio/json.hpp"

namespace sio {
using nlohmann::json;
struct SpeechToTextConfig {
  bool online = true;

  FeatureConfig feature;

  std::string mean_var_norm_file;
  std::string tokenizer;
  std::string model;
  std::string graph;
  std::string context;

  bool do_endpointing = false;

  Error Load(std::string config_file) {
    // Load configs
    std::ifstream is(config_file);
    json jc;
    is >> jc;

    online = jc["online"];
    feature.feature_type = jc["feature"]["type"];
    feature.fbank_config = jc["feature"]["fbank_config"];
    mean_var_norm_file = jc["mean_var_norm"];

    return Error::OK;
  }

}; // class SpeechToTextConfig
}  // namespace sio
#endif
