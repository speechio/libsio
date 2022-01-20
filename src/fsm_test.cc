#include <gtest/gtest.h>
#include "sio/fsm.h"

//#include "sio/dbg.h"

namespace sio {
TEST(Fsm, Basic) {
    Fsm fsa;
    EXPECT_EQ(fsa.NumStates(), 0);
    EXPECT_EQ(fsa.NumArcs(), 0);
}
}