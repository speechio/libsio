#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include "sio/ptr.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"

namespace sio {

template <typename BeamSearchGraphT>
struct SpeechToText {
  const SpeechToTextConfig &config;
  FeatureInfo feature_info;

  SpeechToText(const SpeechToTextConfig& c) :
    config(c),
    feature_info(c.feature_config)
  { }

  Optional<Recognizer*> Create() {
    return new Recognizer(feature_info);
  }

  int Destroy(Recognizer* stt) { 
    delete stt;
    return 0;
  }

//  // acoustic model
//  TransitionModel trans_model_;
//  nnet3::AmNnetSimple am_nnet_;
//  nnet3::DecodableNnetSimpleLoopedInfo *decodable_info_;
//
//  // decoding graph
//  BeamSearchGraphT *beam_search_graph_;
//  fst::SymbolTable *word_syms_;
//
//  // context
//  ContextManager *context_manager_;
}; // class SpeechToText
}  // namespace sio

#endif
