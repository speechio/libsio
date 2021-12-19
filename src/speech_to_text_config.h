#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include "sio/feature.h"
#include "sio/json.h"

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

  Error Load(std::string json_config) {
    // Load configs
    json::JSON jc = json::Load(json_config);

    online = jc["online"].ToBool();
    feature.feature_type = jc["feature"]["type"].ToString();
    feature.fbank_config = jc["feature"]["fbank_config"].ToString();
    mean_var_norm_file = jc["mean_var_norm"].ToString();

    return Error::OK;
  }

}; // class SpeechToTextConfig
}  // namespace sio
#endif
