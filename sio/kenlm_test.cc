#include "sio/kenlm.h"

#include <gtest/gtest.h>
#include <fstream>

namespace sio {

TEST(KenLM, Basic) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/model/tokenizer.vocab");

    KenLM lm;
    lm.Load("testdata/model/lm.trie", tokenizer);

    std::ifstream test_cases("testdata/sentences.txt");

    Str sentence;
    while(std::getline(test_cases, sentence)) {
        Vec<Str> words = absl::StrSplit(sentence, " ");
        Str log = "[KenLM]";
        
        KenLM::State state[2];
        KenLM::State* is = &state[0];
        KenLM::State* os = &state[1];

        lm.SetStateToNull(is);
        for (const auto& w : words) {
            f32 score = lm.GetScore(is, lm.GetWordIndex(w), os);
            log += " " + w + "[" + std::to_string(lm.GetWordIndex(w)) + "]=" + std::to_string(score);
            std::swap(is, os);
        }
        SIO_INFO << log;
    }
}

} // namespace sio
