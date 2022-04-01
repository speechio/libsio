#include "sio/language_model.h"

#include "gtest/gtest.h"

namespace sio {

TEST(LanguageModel, PrefixTreeLm) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/tokenizer.vocab");
    //dbg(tokenizer.index_to_token);

    PrefixTreeLm prefix_lm;
    LanguageModel* lm = &prefix_lm;

    LmStateId null_state = lm->NullState();
    EXPECT_EQ(null_state, 0); // null state is index as 0

    LmStateId bos_state;
    lm->GetScore(null_state, tokenizer.Index("<s>"), &bos_state);

    LmStateId a;
    lm->GetScore(bos_state, tokenizer.Index("a"), &a);

    LmStateId aa;
    lm->GetScore(a, tokenizer.Index("a"), &aa);

    LmStateId ab;
    lm->GetScore(a, tokenizer.Index("b"), &ab);

    LmStateId ab_eos;
    lm->GetScore(ab, tokenizer.Index("</s>"), &ab_eos);
}


TEST(LanguageModel, KenLm) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/model/tokenizer.vocab");

    KenLm lm;
    lm.Load("testdata/model/lm.trie", tokenizer);

    std::ifstream sentences("testdata/sentences.txt");

    Str sentence;
    while(std::getline(sentences, sentence)) {
        Vec<Str> words = absl::StrSplit(sentence, " ");
        Str log = "[KenLm]";
        
        KenLm::State state[2];
        KenLm::State* is = &state[0];
        KenLm::State* os = &state[1];

        lm.SetStateToNull(is);
        for (const auto& w : words) {
            f32 score = lm.Score(is, lm.GetWordIndex(w), os);
            log += " " + w + "[" + std::to_string(lm.GetWordIndex(w)) + "]=" + std::to_string(score);
            std::swap(is, os);
        }
        SIO_INFO << log;
    }
}


TEST(LanguageModel, NgramLm) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/model/tokenizer.vocab");

    KenLm kenlm;
    kenlm.Load("testdata/model/lm.trie", tokenizer);

    NgramLm ngram;
    ngram.Load(kenlm);

    ScaleCacheLm lm;
    lm.Load(ngram, 1.0);

    std::ifstream sentences("testdata/sentences.txt");

    Str sentence;
    while(std::getline(sentences, sentence)) {
        Vec<Str> words = absl::StrSplit(sentence, " ");
        Str log = "[NgramLm]";
        
        LmStateId state[2];
        LmStateId* is = &state[0];
        LmStateId* os = &state[1];

        *is = lm.NullState();
        for (const auto& w : words) {
            f32 score = lm.GetScore(*is, tokenizer.Index(w), os);
            log += " " + w + "[" + std::to_string(tokenizer.Index(w)) + "]=" + std::to_string(score);
            std::swap(is, os);
        }
        SIO_INFO << log;
    }
}

} // namespace sio
