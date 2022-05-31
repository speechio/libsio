#ifndef SIO_SPEECH_TO_TEXT_CONFIG_H
#define SIO_SPEECH_TO_TEXT_CONFIG_H

#include <fstream>

#include "sio/base.h"
#include "sio/struct_loader.h"
#include "sio/feature_extractor.h"
#include "sio/scorer.h"
#include "sio/language_model.h"

namespace sio {
struct SpeechToTextConfig {
    bool online = true;

    FeatureConfig feature;
    std::string mean_var_norm;

    std::string tokenizer_vocab;
    std::string tokenizer_model;

    std::string nnet;
    ScorerConfig scorer;

    std::string graph;
    std::string context;
    bool do_endpointing = false;

    BeamSearchConfig beam_search;

    std::string language_model;

    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".online", &online);

        feature.Register(loader, module + ".feature");
        loader->AddEntry(module + ".mean_var_norm", &mean_var_norm);

        loader->AddEntry(module + ".tokenizer.vocab", &tokenizer_vocab);
        loader->AddEntry(module + ".tokenizer.model", &tokenizer_model);

        loader->AddEntry(module + ".nnet", &nnet);
        scorer.Register(loader, module + ".scorer");

        loader->AddEntry(module + ".graph", &graph);
        loader->AddEntry(module + ".context", &context);
        loader->AddEntry(module + ".do_endpointing", &do_endpointing);

        beam_search.Register(loader, module + ".beam_search");

        loader->AddEntry(module + ".language_model", &language_model);

        return Error::OK;
    }


    Error Load(const std::string& config_file) {
        StructLoader loader;
        Register(&loader, "stt");
        loader.Load(config_file);
        loader.Print();

        return Error::OK;
    }

}; // class SpeechToTextConfig
}  // namespace sio
#endif
