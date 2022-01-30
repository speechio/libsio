#ifndef SIO_SPEECH_TO_TEXT_MODEL_H
#define SIO_SPEECH_TO_TEXT_MODEL_H

#include <torch/script.h>

#include "sio/common.h"
#include "sio/tokenizer.h"
#include "sio/speech_to_text_config.h"

namespace sio {
struct SpeechToTextModel {
    SpeechToTextConfig config;
    torch::jit::script::Module nnet;
    Tokenizer tokenizer;
    Unique<MeanVarNorm*> mean_var_norm;


    Error Load(std::string config_file) { 
        config.Load(config_file);

        if (config.mean_var_norm != "") {
            SIO_CHECK(!mean_var_norm) << "mean_var_norm initialized already.";
            mean_var_norm = make_unique<MeanVarNorm>();
            mean_var_norm->Load(config.mean_var_norm);
        } else {
            mean_var_norm.reset();
        }

        tokenizer.Load(config.tokenizer_vocab);

        SIO_CHECK(config.nnet != "") << "stt nnet is required";
        SIO_INFO << "Loading torchscript nnet from: " << config.nnet; 
        nnet = torch::jit::load(config.nnet);

        return Error::OK;
    }

}; // class SpeechToTextModel
}  // namespace sio

#endif
