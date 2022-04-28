#include "sio/base.h"

#include "gtest/gtest.h"

TEST(dbg_macro, Basic) {
    using namespace sio;
    int i = 0, j = 0;
    while(dbg(i < 2)) { // with DBG_MACRO_DISABLE, dbg() is no-op
        dbg(i++);
        j--;
    }
    EXPECT_EQ(i,  2);
    EXPECT_EQ(dbg("j value check in EXPECT_EQ:", j), -2); // comma expression takes last one as its value

    hashmap<i32, str> m = {
        {1,"abc"}, 
        {2,"def"}
    };

    vec<str> v = {"s1", "s2", "s3"};

    dbg("---- dbg macro ----", i, m, v, "==== dbg macro ====");
}

