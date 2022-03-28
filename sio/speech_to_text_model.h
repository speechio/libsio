#ifndef SIO_SPEECH_TO_TEXT_MODEL_H
#define SIO_SPEECH_TO_TEXT_MODEL_H

#include <fstream>

#include <torch/script.h>

#include "sio/common.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_machine.h"
#include "sio/speech_to_text_config.h"

namespace sio {
/*
 * SpeechToTextModel stores stateless resources, 
 * can be shared by different threads.
 * TODO: check torchscript multi-thread usage.
 */
struct SpeechToTextModel {
    SpeechToTextConfig config;

    Unique<MeanVarNorm*> mean_var_norm;

    Tokenizer tokenizer;

    torch::jit::script::Module nnet;

    Fsm graph;

    Error Load(std::string config_file) { 
        config.Load(config_file);

        if (config.mean_var_norm != "") {
            SIO_CHECK(!mean_var_norm);
            mean_var_norm = std::make_unique<MeanVarNorm>();
            mean_var_norm->Load(config.mean_var_norm);
        } else {
            mean_var_norm.reset();
        }

        tokenizer.Load(config.tokenizer_vocab);

        SIO_CHECK(config.nnet != "");
        SIO_INFO << "Loading torchscript nnet from: " << config.nnet; 
        nnet = torch::jit::load(config.nnet);

        if (config.graph != "") {
            SIO_INFO << "Loading decoding graph from: " << config.graph;
            std::ifstream is(config.graph, std::ios::binary);
            SIO_CHECK(is.good());
            graph.LoadFromBinary(is);
        } else {
            SIO_INFO << "Building decoding graph from: " << config.tokenizer_vocab;
            graph.BuildTokenTopology(tokenizer);
        }

        return Error::OK;
    }

}; // class SpeechToTextModel
}  // namespace sio

#endif
