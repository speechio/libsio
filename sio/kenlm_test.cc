#include "sio/kenlm.h"

#include <gtest/gtest.h>
#include <fstream>

namespace sio {

TEST(KenLM, Basic) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/model/tokenizer.vocab");

    KenLM lm;
    lm.Load("testdata/model/lm.trie", tokenizer);

    std::ifstream is("testdata/sentences.txt");

    Str sentence;
    while(std::getline(is, sentence)) {
        Vec<Str> words = absl::StrSplit(sentence, " ");
        Str log = "[KenLM]";
        
        KenLM::State state[2];
        KenLM::State* istate = &state[0];
        KenLM::State* ostate = &state[1];

        lm.SetStateToNull(istate);
        for (const auto& w : words) {
            f32 score = lm.Score(istate, lm.GetWordIndex(w), ostate);
            log += " " + w + "[" + std::to_string(lm.GetWordIndex(w)) + "]=" + std::to_string(score);
            std::swap(istate, ostate);
        }
        SIO_INFO << log;
    }
}

} // namespace sio
