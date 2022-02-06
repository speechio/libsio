#include "gtest/gtest.h"
#include "sio/language_model.h"
#include "sio/dbg.h"

namespace sio {

TEST(LanguageModel, PrefixLm) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/tokenizer.vocab");
    //dbg(tokenizer.index_to_token);

    PrefixLm prefix_lm;
    prefix_lm.Load(tokenizer);

    LanguageModel* m = &prefix_lm;

    LmStateId null_state = m->NullLmState();
    EXPECT_EQ(null_state, 0); // null state is index as 0

    LmScore s;

    LmStateId a;
    m->GetLmScore(null_state, tokenizer.Index("a"), &s, &a);
    //dbg(null_state, s, a);
    EXPECT_EQ(a, 1);

    LmStateId ab;
    m->GetLmScore(a, tokenizer.Index("b"), &s, &ab);
    //dbg(a, s, ab);
    EXPECT_EQ(ab, 2);

    LmStateId aba;
    m->GetLmScore(ab, tokenizer.Index("a"), &s, &aba);
    //dbg(ab, s, aba);
    EXPECT_EQ(aba, 3);

    LmStateId abb;
    m->GetLmScore(ab, tokenizer.Index("b"), &s, &abb);
    //dbg(ab, s, abb);
    EXPECT_EQ(abb, 4);

}

} // namespace sio
