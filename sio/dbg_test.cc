//#define DBG_MACRO_DISABLE
#include "sio/dbg.h"

#include "gtest/gtest.h"

#include "sio/base.h"

TEST(dbg_macro, Basic) {
    using namespace sio;
    int i = 0, j = 0;
    while(dbg(i < 2)) { // with DBG_MACRO_DISABLE, dbg() is no-op
        dbg(i++);
        j--;
    }
    EXPECT_EQ(i,  2);
    EXPECT_EQ(dbg("j value check in EXPECT_EQ:", j), -2); // comma expression takes last one as its value

    Map<i32, Str> m = {
        {1,"abc"}, 
        {2,"def"}
    };

    Vec<Str> v = {"s1", "s2", "s3"};

    dbg("---- dbg macro ----", i, m, v, "==== dbg macro ====");
}
