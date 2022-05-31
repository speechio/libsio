#ifndef SIO_SPEECH_TO_TEXT_MODULE_H
#define SIO_SPEECH_TO_TEXT_MODULE_H

#include <fstream>

#include <torch/script.h>

#include "sio/base.h"
#include "sio/mean_var_norm.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_transducer.h"
#include "sio/kenlm.h"
#include "sio/language_model.h"
#include "sio/speech_to_text_config.h"

namespace sio {
/*
 * SpeechToTextModule holds ownerships of static resources/assets for speech recognition.
 * It is stateless, and can be shared by mulitple runtime threads.
 * TODO: check torchscript multi-thread usage.
 */
struct SpeechToTextModule {
    SpeechToTextConfig config;

    Unique<MeanVarNorm*> mean_var_norm; // need pointer here because MVN is optional

    Tokenizer tokenizer;

    torch::jit::script::Module nnet;

    Fst graph;

    vec<LanguageModelInfo> lms;

    vec<KenLm> kenlms;
    hashmap<str, int> kenlms_map;

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

        if (config.language_model != "") {
            SIO_INFO << "Loading language models from: " << config.language_model;

            std::ifstream lm_config(config.language_model);
            SIO_CHECK(lm_config.good());

            str line;
            while(std::getline(lm_config, line)) {
                if (absl::StartsWith(line, "#")) {
                    continue;
                }

                lms.emplace_back();
                LanguageModelInfo& lm_info = lms.back();
                lm_info.Load(Json::parse(line));

                if (lm_info.type == LanguageModelType::KEN_LM) {
                    kenlms.emplace_back();
                    kenlms.back().Load(lm_info.path, tokenizer);
                    kenlms_map.insert({lm_info.name, kenlms.size()-1});
                } else {
                    ; // TODO
                }

                SIO_INFO << "    LM loaded: " 
                         << lm_info.name  << " " << lm_info.path << " " 
                         << lm_info.scale << " " << lm_info.cache;
            }

            SIO_INFO << "Total language models: " << lms.size();
        }

        return Error::OK;
    }

}; // class SpeechToTextModule
}  // namespace sio

#endif
