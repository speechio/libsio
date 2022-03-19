#include "sio/language_model.h"

#include "gtest/gtest.h"

//#include "sio/dbg.h"

namespace sio {

TEST(LanguageModel, PrefixLm) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/tokenizer.vocab");
    //dbg(tokenizer.index_to_token);

    PrefixTreeLM prefix_lm;
    LanguageModel* lm = &prefix_lm;

    LmStateId null_state = lm->NullState();
    EXPECT_EQ(null_state, 0); // null state is index as 0

    LmScore s;

    LmStateId bos_state;
    lm->GetScore(null_state, tokenizer.Index("<s>"), &s, &bos_state);

    LmStateId a;
    lm->GetScore(bos_state, tokenizer.Index("a"), &s, &a);

    LmStateId aa;
    lm->GetScore(a, tokenizer.Index("a"), &s, &aa);

    LmStateId ab;
    lm->GetScore(a, tokenizer.Index("b"), &s, &ab);

    LmStateId ab_eos;
    lm->GetScore(ab, tokenizer.Index("</s>"), &s, &ab_eos);
}

} // namespace sio
