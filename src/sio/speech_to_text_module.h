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
    vec<Context> contexts;

    // choose unordered_map instead of vector because:
    //   rehash retains element memory address stability, realloc doesn't
    hashmap<str, KenLm> kenlms;

    Error Load(std::string stt_config) { 
        config.Load(stt_config);

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

        if (config.contexts != "") {
            SIO_INFO << "Loading contexts from: " << config.contexts;

            std::ifstream contexts_stream(config.contexts);
            SIO_CHECK(contexts_stream.good());

            str line;
            while(std::getline(contexts_stream, line)) {
                if (absl::StartsWith(line, "#")) {
                    continue;
                }

                contexts.emplace_back();
                contexts.back().Load(Json::parse(line));

                const Context& c = contexts.back();
                if (c.type == LmType::PrefixTreeLm) {
                    ; // PrefixTreeLm doesn't have static resources
                } else if (c.type == LmType::KenLm) {
                    SIO_CHECK(kenlms.find(c.name) == kenlms.end());
                    KenLm m;
                    m.Load(c.path, tokenizer);
                    kenlms[c.name] = std::move(m);
                } else if (c.type == LmType::FstLm) {
                    ; // TODO
                } else {
                    SIO_PANIC(Error::Unreachable);
                }

                SIO_INFO << "    Context LM loaded: " 
                         << c.name <<" "<< c.major <<" "<< c.path <<" "<< c.scale <<" "<< c.cache;
            }

            SIO_INFO << "    Total contexts: " << contexts.size();
        }

        return Error::OK;
    }

}; // class SpeechToTextModule
}  // namespace sio

#endif
