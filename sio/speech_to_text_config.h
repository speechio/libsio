#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include <fstream>

#include "sio/common.h"
#include "sio/struct_loader.h"
#include "sio/feature_extractor.h"
#include "sio/scorer.h"

namespace sio {
struct SpeechToTextConfig {
    bool online = true;

    FeatureExtractorConfig feature_extractor;
    std::string mean_var_norm;

    std::string tokenizer_vocab;
    std::string tokenizer_model;

    std::string nnet;
    ScorerConfig scorer;

    std::string graph;
    std::string context;
    bool do_endpointing = false;


    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".online", &online);

        feature_extractor.Register(loader, module + ".feature_extractor");
        loader->AddEntry(module + ".mean_var_norm", &mean_var_norm);

        loader->AddEntry(module + ".tokenizer.vocab", &tokenizer_vocab);
        loader->AddEntry(module + ".tokenizer.model", &tokenizer_model);

        loader->AddEntry(module + ".nnet", &nnet);
        scorer.Register(loader, module + ".scorer");

        loader->AddEntry(module + ".graph", &graph);
        loader->AddEntry(module + ".context", &context);
        loader->AddEntry(module + ".do_endpointing", &do_endpointing);

        return Error::OK;
    }


    Error Load(const std::string& config_file) {
        StructLoader loader;
        Register(&loader);
        loader.Load(config_file);
        loader.Print();

        return Error::OK;
    }

}; // class SpeechToTextConfig
}  // namespace sio
#endif
