#include "sio/kenlm.h"

#include <gtest/gtest.h>

namespace sio {

TEST(KenLM, Load) {
    Tokenizer tokenizer;
    tokenizer.Load("../exp/stt_zh/tokenizer.vocab");

    KenLM kenlm;
    kenlm.Load("../exp/lm/lm.trie", tokenizer);
}

} // namespace sio
