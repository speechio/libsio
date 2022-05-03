#include "sio/fst.h"

#include <gtest/gtest.h>
#include <fstream>

namespace sio {
TEST(Fst, Basic) {
    Fst fst;
    std::ifstream is("testdata/token_topo.int");
    fst.LoadFromText(is);
    {
        std::ofstream os("testdata/T.fst", std::ios::binary);
        fst.Dump(os);
    }


    Fst fst2;
    std::ifstream is2("testdata/T.fst", std::ios::binary);
    fst2.Load(is2);
    EXPECT_EQ(fst2.num_states, 4);
    EXPECT_EQ(fst2.num_arcs, 8);
    EXPECT_EQ(fst2.start_state, 0);
    EXPECT_EQ(fst2.final_state, 3);
    fst2.DumpToText(std::cout);
    {
        std::ofstream os("testdata/T2.fst", std::ios::binary);
        fst2.Dump(os);
    }


    Fst fst3;
    Tokenizer tokenizer;
    tokenizer.Load("testdata/tokenizer.vocab");
    fst3.BuildTokenTopology(tokenizer);
    fst3.DumpToText(std::cout);
    {
        std::ofstream os("testdata/T3.fst", std::ios::binary);
        fst3.Dump(os);
    }

}
} // namespace sio
