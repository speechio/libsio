#include "sio/finite_state_machine.h"

#include <gtest/gtest.h>
#include <fstream>

//#include "sio/dbg.h"

namespace sio {
TEST(Fsm, Basic) {
    Fsm fsm;
    std::ifstream is("testdata/token_topo.int");
    fsm.LoadFromStream(is);
    {
        std::ofstream os("testdata/T.fsm", std::ios::binary);
        fsm.Dump(os);
    }


    Fsm fsm2;
    std::ifstream is2("testdata/T.fsm", std::ios::binary);
    fsm2.Load(is2);
    EXPECT_EQ(fsm2.num_states, 4);
    EXPECT_EQ(fsm2.num_arcs, 8);
    EXPECT_EQ(fsm2.start_state, 0);
    EXPECT_EQ(fsm2.final_state, 3);
    fsm2.Print();
    {
        std::ofstream os("testdata/T2.fsm", std::ios::binary);
        fsm2.Dump(os);
    }


    Fsm fsm3;
    Tokenizer tokenizer;
    //tokenizer.Load("testdata/tmp_tokenizer.vocab");
    tokenizer.Load("testdata/tokenizer.vocab");
    fsm3.BuildTokenTopology(tokenizer);
    fsm3.Print();
    {
        std::ofstream os("testdata/T3.fsm", std::ios::binary);
        fsm3.Dump(os);
    }

/*
    Fsm ctc_graph;
    Tokenizer t;
    t.Load("model/tokenizer.vocab");
    ctc_graph.BuildTokenTopology(t);
    ctc_graph.Print();
    {
        std::ofstream os("model/compact_ctc.fsm", std::ios::binary);
        ctc_graph.Dump(os);
    }
*/
}
}
