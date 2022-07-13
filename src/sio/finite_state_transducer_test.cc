#include "sio/finite_state_transducer.h"

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
    EXPECT_EQ(fst2.states.size(), 4);
    EXPECT_EQ(fst2.arcs.size(), 8);
    EXPECT_EQ(fst2.start_state, 0);
    EXPECT_EQ(fst2.final_state, 3);
    fst2.DumpToText(std::cout);
    {
        std::ofstream os("testdata/T2.fst", std::ios::binary);
        fst2.Dump(os);
    }

}
} // namespace sio
