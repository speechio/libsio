#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include <torch/script.h>

#include "sio/common.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"
#include "sio/tokenizer.h"

namespace sio {

struct SpeechToText {
    SpeechToTextConfig config;
    torch::jit::script::Module nnet;
    Tokenizer tokenizer;
    Optional<Owner<MeanVarNorm*>> mean_var_norm = nullptr;


    ~SpeechToText() noexcept { 
        if (mean_var_norm != nullptr) {
            delete mean_var_norm; mean_var_norm = nullptr;
        }
    }


    Error Setup(std::string config_file) { 
        config.Setup(config_file);

        if (config.mean_var_norm_file != "") {
            mean_var_norm = new MeanVarNorm;
            mean_var_norm->Setup(config.mean_var_norm_file);
        } else {
            mean_var_norm = nullptr;
        }

        tokenizer.Setup(config.tokenizer_vocab);

        SIO_CHECK(config.nnet != "") << "stt nnet is required";
        SIO_INFO << "Loading torchscript nnet from: " << config.nnet; 
        nnet = torch::jit::load(config.nnet);

        return Error::OK;
    }


    Optional<Recognizer*> CreateRecognizer() {
        try {
            Recognizer* rec = new Recognizer;
            rec->Setup(
                config.feature_extractor, mean_var_norm, /* feature */
                tokenizer, /* tokenizer */ 
                config.scorer, nnet /* scorer */
            ); 
            return rec;
        } catch (...) {
            return nullptr;
        }
    }


    void DestroyRecognizer(Recognizer* rec) {
        SIO_CHECK(rec != nullptr);
        delete rec;
    }

}; // class SpeechToText
}  // namespace sio

#endif
