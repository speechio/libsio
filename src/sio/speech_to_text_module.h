#ifndef SIO_SPEECH_TO_TEXT_MODULE_H
#define SIO_SPEECH_TO_TEXT_MODULE_H

#include <fstream>

#include <torch/script.h>

#include "sio/base.h"
#include "sio/mean_var_norm.h"
#include "sio/tokenizer.h"
#include "sio/finite_state_transducer.h"
#include "sio/token_topology.h"
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
    Unique<MeanVarNorm*> mean_var_norm; // pointer here because MVN is optional
    Tokenizer tokenizer;
    torch::jit::script::Module nnet;
    Fst graph;
    vec<Context> contexts;


    Error Load(std::string stt_config) { 
        config.Load(stt_config);

        if (config.mean_var_norm != "") {
            SIO_INFO << "Loading MVN from: " << config.mean_var_norm; 
            SIO_CHECK(!mean_var_norm);
            mean_var_norm = std::make_unique<MeanVarNorm>();
            mean_var_norm->Load(config.mean_var_norm);
        } else {
            mean_var_norm.reset();
        }

        SIO_INFO << "Loading tokenizer from: " << config.tokenizer_vocab; 
        tokenizer.Load(config.tokenizer_vocab);

        SIO_CHECK(config.nnet != "");
        SIO_INFO << "Loading nnet model from: " << config.nnet; 
        nnet = torch::jit::load(config.nnet);

        if (config.graph != "") {
            SIO_INFO << "Loading decoding graph from: " << config.graph;
            std::ifstream is(config.graph, std::ios::binary);
            SIO_CHECK(is.good());
            graph.Load(is);
        } else {
            SIO_INFO << "Building decoding graph(T.fst) from tokenizer of size:" << tokenizer.Size();
            BuildTokenTopology(tokenizer, &graph);
        }

        if (config.contexts != "") {
            SIO_INFO << "Loading contexts from: " << config.contexts;

            std::ifstream contexts_stream(config.contexts);
            SIO_CHECK(contexts_stream.good());

            str line;
            while(std::getline(contexts_stream, line)) {
                if (absl::StartsWith(line, "#")) {
                    continue;
                }
                Json info = Json::parse(line);

                contexts.emplace_back();
                Context& c = contexts.back();

                if (info.at("type") == "PrefixTreeLm") {
                    c.type = LmType::PrefixTreeLm;
                } else if (info.at("type") == "KenLm") {
                    c.type = LmType::KenLm;
                } else if (info.at("type") == "FstLm") {
                    c.type = LmType::FstLm;
                }

                switch (c.type) {
                    case LmType::PrefixTreeLm:
                        c.name = info.value("name", info.at("type"));
                        c.tags = info.value("tags", "");

                        SIO_INFO << "    context loaded: " << c.name;
                        break;

                    case LmType::KenLm:
                        c.name = info.value("name", info.at("type"));
                        c.tags = info.value("tags", "");

                        c.path = info.at("path");
                        c.scale = info.value("scale", 0.0);
                        c.cache = info.value("cache", 0);

                        c.kenlm = std::make_unique<KenLm>();
                        c.kenlm->Load(c.path, tokenizer);

                        SIO_INFO << "    context loaded: " << c.name <<" "<< c.path;
                        break;

                    case LmType::FstLm:
                        break;

                    default:
                        SIO_PANIC(Error::UnsupportedLanguageModel);
                }
            }
            SIO_INFO << "    total contexts: " << contexts.size();
        }
        return Error::OK;
    }

}; // class SpeechToTextModule
}  // namespace sio

#endif
