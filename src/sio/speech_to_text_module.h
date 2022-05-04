#ifndef SIO_SPEECH_TO_TEXT_MODULE_H
#define SIO_SPEECH_TO_TEXT_MODULE_H

#include <fstream>

#include <torch/script.h>

#include "sio/base.h"
#include "sio/mean_var_norm.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_transducer.h"
#include "sio/speech_to_text_config.h"

namespace sio {
/*
 * SpeechToTextModule loads & stores stateless resources, can be shared by different runtime threads.
 * TODO: check torchscript multi-thread usage.
 */
struct SpeechToTextModule {
    SpeechToTextConfig config;

    Unique<MeanVarNorm*> mean_var_norm; // need pointer here because MVN is optional

    Tokenizer tokenizer;

    torch::jit::script::Module nnet;

    Fst graph;

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
            graph.Load(is);
        } else {
            SIO_INFO << "Building decoding graph from: " << config.tokenizer_vocab;
            graph.BuildTokenTopology(tokenizer);
        }

        return Error::OK;
    }

}; // class SpeechToTextModule
}  // namespace sio

#endif
