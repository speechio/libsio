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
    {
        std::ifstream is2("testdata/graph.fsm", std::ios::binary);
        fsm2.Load(is2);
    }

    EXPECT_EQ(fsm2.NumStates(), 4);
    EXPECT_EQ(fsm2.NumArcs(), 8);
    EXPECT_EQ(fsm2.Start(), 0);
    EXPECT_EQ(fsm2.Final(), 3);

    printf("%d,%d,%d,%d\n", fsm2.NumStates(), fsm2.NumArcs(), fsm2.Start(), fsm2.Final());
    for (Fsm::StateId s = 0; s < fsm2.NumStates(); s++) {
        for (auto ai = fsm2.GetArcIterator(s); !ai.Done(); ai.Next()) {
            const Fsm::Arc& arc = ai.Value();
            printf("%d\t%d\t%d:%d\t%f\n", arc.src, arc.dst, arc.label, arc.aux.label, arc.weight);
        }
    }
}
}