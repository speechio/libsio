#include <gtest/gtest.h>

#include <fstream>
#include "sio/fsm.h"

//#include "sio/dbg.h"

namespace sio {
TEST(Fsm, Basic) {
    Fsm fsm;
    std::ifstream is("testdata/graph.int");
    fsm.LoadFromString(is);
    {
        std::ofstream os("testdata/graph.fsm", std::ios::binary);
        fsm.Dump(os);
    }


    Fsm fsm2;
    std::ifstream is2("testdata/graph.fsm", std::ios::binary);
    fsm2.Load(is2);
    EXPECT_EQ(fsm2.NumStates(), 4);
    EXPECT_EQ(fsm2.NumArcs(), 8);
    EXPECT_EQ(fsm2.Start(), 0);
    EXPECT_EQ(fsm2.Final(), 3);
    fsm2.Print();
    {
        std::ofstream os("testdata/graph.fsm2", std::ios::binary);
        fsm2.Dump(os);
    }


    Fsm fsm3;
    Tokenizer tokenizer;
    tokenizer.Load("testdata/tokenizer.vocab");
    fsm3.BuildTokenTopo(tokenizer);
    fsm3.Print();
    {
        std::ofstream os("testdata/graph.fsm3", std::ios::binary);
        fsm3.Dump(os);
    }
}
}
